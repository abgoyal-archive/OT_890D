
#include "../rt_config.h"
// WPA OUI
UCHAR		OUI_WPA_NONE_AKM[4]		= {0x00, 0x50, 0xF2, 0x00};
UCHAR       OUI_WPA_VERSION[4]      = {0x00, 0x50, 0xF2, 0x01};
UCHAR       OUI_WPA_WEP40[4]      = {0x00, 0x50, 0xF2, 0x01};
UCHAR       OUI_WPA_TKIP[4]     = {0x00, 0x50, 0xF2, 0x02};
UCHAR       OUI_WPA_CCMP[4]     = {0x00, 0x50, 0xF2, 0x04};
UCHAR       OUI_WPA_WEP104[4]      = {0x00, 0x50, 0xF2, 0x05};
UCHAR       OUI_WPA_8021X_AKM[4]	= {0x00, 0x50, 0xF2, 0x01};
UCHAR       OUI_WPA_PSK_AKM[4]      = {0x00, 0x50, 0xF2, 0x02};
// WPA2 OUI
UCHAR       OUI_WPA2_WEP40[4]   = {0x00, 0x0F, 0xAC, 0x01};
UCHAR       OUI_WPA2_TKIP[4]        = {0x00, 0x0F, 0xAC, 0x02};
UCHAR       OUI_WPA2_CCMP[4]        = {0x00, 0x0F, 0xAC, 0x04};
UCHAR       OUI_WPA2_8021X_AKM[4]   = {0x00, 0x0F, 0xAC, 0x01};
UCHAR       OUI_WPA2_PSK_AKM[4]   	= {0x00, 0x0F, 0xAC, 0x02};
UCHAR       OUI_WPA2_WEP104[4]   = {0x00, 0x0F, 0xAC, 0x05};
// MSA OUI
UCHAR   	OUI_MSA_8021X_AKM[4]    = {0x00, 0x0F, 0xAC, 0x05};		// Not yet final - IEEE 802.11s-D1.06
UCHAR   	OUI_MSA_PSK_AKM[4]   	= {0x00, 0x0F, 0xAC, 0x06};		// Not yet final - IEEE 802.11s-D1.06

VOID	PRF(
	IN	UCHAR	*key,
	IN	INT		key_len,
	IN	UCHAR	*prefix,
	IN	INT		prefix_len,
	IN	UCHAR	*data,
	IN	INT		data_len,
	OUT	UCHAR	*output,
	IN	INT		len)
{
	INT		i;
    UCHAR   *input;
	INT		currentindex = 0;
	INT		total_len;

	// Allocate memory for input
	os_alloc_mem(NULL, (PUCHAR *)&input, 1024);

    if (input == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("!!!PRF: no memory!!!\n"));
        return;
    }

	// Generate concatenation input
	NdisMoveMemory(input, prefix, prefix_len);

	// Concatenate a single octet containing 0
	input[prefix_len] =	0;

	// Concatenate specific data
	NdisMoveMemory(&input[prefix_len + 1], data, data_len);
	total_len =	prefix_len + 1 + data_len;

	// Concatenate a single octet containing 0
	// This octet shall be update later
	input[total_len] = 0;
	total_len++;

	// Iterate to calculate the result by hmac-sha-1
	// Then concatenate to last result
	for	(i = 0;	i <	(len + 19) / 20; i++)
	{
		HMAC_SHA1(input, total_len,	key, key_len, &output[currentindex]);
		currentindex +=	20;

		// update the last octet
		input[total_len - 1]++;
	}
    os_free_mem(NULL, input);
}

VOID WpaCountPTK(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	*PMK,
	IN	UCHAR	*ANonce,
	IN	UCHAR	*AA,
	IN	UCHAR	*SNonce,
	IN	UCHAR	*SA,
	OUT	UCHAR	*output,
	IN	UINT	len)
{
	UCHAR	concatenation[76];
	UINT	CurrPos = 0;
	UCHAR	temp[32];
	UCHAR	Prefix[] = {'P', 'a', 'i', 'r', 'w', 'i', 's', 'e', ' ', 'k', 'e', 'y', ' ',
						'e', 'x', 'p', 'a', 'n', 's', 'i', 'o', 'n'};

	// initiate the concatenation input
	NdisZeroMemory(temp, sizeof(temp));
	NdisZeroMemory(concatenation, 76);

	// Get smaller address
	if (RTMPCompareMemory(SA, AA, 6) == 1)
		NdisMoveMemory(concatenation, AA, 6);
	else
		NdisMoveMemory(concatenation, SA, 6);
	CurrPos += 6;

	// Get larger address
	if (RTMPCompareMemory(SA, AA, 6) == 1)
		NdisMoveMemory(&concatenation[CurrPos], SA, 6);
	else
		NdisMoveMemory(&concatenation[CurrPos], AA, 6);

	// store the larger mac address for backward compatible of
	// ralink proprietary STA-key issue
	NdisMoveMemory(temp, &concatenation[CurrPos], MAC_ADDR_LEN);
	CurrPos += 6;

	// Get smaller Nonce
	if (RTMPCompareMemory(ANonce, SNonce, 32) == 0)
		NdisMoveMemory(&concatenation[CurrPos], temp, 32);	// patch for ralink proprietary STA-key issue
	else if (RTMPCompareMemory(ANonce, SNonce, 32) == 1)
		NdisMoveMemory(&concatenation[CurrPos], SNonce, 32);
	else
		NdisMoveMemory(&concatenation[CurrPos], ANonce, 32);
	CurrPos += 32;

	// Get larger Nonce
	if (RTMPCompareMemory(ANonce, SNonce, 32) == 0)
		NdisMoveMemory(&concatenation[CurrPos], temp, 32);	// patch for ralink proprietary STA-key issue
	else if (RTMPCompareMemory(ANonce, SNonce, 32) == 1)
		NdisMoveMemory(&concatenation[CurrPos], ANonce, 32);
	else
		NdisMoveMemory(&concatenation[CurrPos], SNonce, 32);
	CurrPos += 32;

	hex_dump("concatenation=", concatenation, 76);

	// Use PRF to generate PTK
	PRF(PMK, LEN_MASTER_KEY, Prefix, 22, concatenation, 76, output, len);

}

VOID	GenRandom(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			*macAddr,
	OUT	UCHAR			*random)
{
	INT		i, curr;
	UCHAR	local[80], KeyCounter[32];
	UCHAR	result[80];
	ULONG	CurrentTime;
	UCHAR	prefix[] = {'I', 'n', 'i', 't', ' ', 'C', 'o', 'u', 'n', 't', 'e', 'r'};

	// Zero the related information
	NdisZeroMemory(result, 80);
	NdisZeroMemory(local, 80);
	NdisZeroMemory(KeyCounter, 32);

	for	(i = 0;	i <	32;	i++)
	{
		// copy the local MAC address
		COPY_MAC_ADDR(local, macAddr);
		curr =	MAC_ADDR_LEN;

		// concatenate the current time
		NdisGetSystemUpTime(&CurrentTime);
		NdisMoveMemory(&local[curr],  &CurrentTime,	sizeof(CurrentTime));
		curr +=	sizeof(CurrentTime);

		// concatenate the last result
		NdisMoveMemory(&local[curr],  result, 32);
		curr +=	32;

		// concatenate a variable
		NdisMoveMemory(&local[curr],  &i,  2);
		curr +=	2;

		// calculate the result
		PRF(KeyCounter, 32, prefix,12, local, curr, result, 32);
	}

	NdisMoveMemory(random, result,	32);
}

static VOID RTMPInsertRsnIeCipher(
	IN  PRTMP_ADAPTER   pAd,
	IN	UCHAR			ElementID,
	IN	UINT			WepStatus,
	IN	BOOLEAN			bMixCipher,
	IN	UCHAR			FlexibleCipher,
	OUT	PUCHAR			pRsnIe,
	OUT	UCHAR			*rsn_len)
{
	UCHAR	PairwiseCnt;

	*rsn_len = 0;

	// decide WPA2 or WPA1
	if (ElementID == Wpa2Ie)
	{
		RSNIE2	*pRsnie_cipher = (RSNIE2*)pRsnIe;

		// Assign the verson as 1
		pRsnie_cipher->version = 1;

        switch (WepStatus)
        {
        	// TKIP mode
            case Ndis802_11Encryption2Enabled:
                NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA2_TKIP, 4);
                pRsnie_cipher->ucount = 1;
                NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA2_TKIP, 4);
                *rsn_len = sizeof(RSNIE2);
                break;

			// AES mode
            case Ndis802_11Encryption3Enabled:
				if (bMixCipher)
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA2_TKIP, 4);
				else
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA2_CCMP, 4);
                pRsnie_cipher->ucount = 1;
                NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA2_CCMP, 4);
                *rsn_len = sizeof(RSNIE2);
                break;

			// TKIP-AES mix mode
            case Ndis802_11Encryption4Enabled:
                NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA2_TKIP, 4);

				PairwiseCnt = 1;
				// Insert WPA2 TKIP as the first pairwise cipher
				if (MIX_CIPHER_WPA2_TKIP_ON(FlexibleCipher))
				{
                	NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA2_TKIP, 4);
					// Insert WPA2 AES as the secondary pairwise cipher
					if (MIX_CIPHER_WPA2_AES_ON(FlexibleCipher))
					{
                		NdisMoveMemory(pRsnie_cipher->ucast[0].oui + 4, OUI_WPA2_CCMP, 4);
						PairwiseCnt = 2;
					}
				}
				else
				{
					// Insert WPA2 AES as the first pairwise cipher
					NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA2_CCMP, 4);
				}

                pRsnie_cipher->ucount = PairwiseCnt;
                *rsn_len = sizeof(RSNIE2) + (4 * (PairwiseCnt - 1));
                break;
        }

#ifdef CONFIG_STA_SUPPORT
		if ((pAd->OpMode == OPMODE_STA) &&
			(pAd->StaCfg.GroupCipher != Ndis802_11Encryption2Enabled) &&
			(pAd->StaCfg.GroupCipher != Ndis802_11Encryption3Enabled))
		{
			UINT GroupCipher = pAd->StaCfg.GroupCipher;
			switch(GroupCipher)
			{
				case Ndis802_11GroupWEP40Enabled:
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA2_WEP40, 4);
					break;
				case Ndis802_11GroupWEP104Enabled:
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA2_WEP104, 4);
					break;
			}
		}
#endif // CONFIG_STA_SUPPORT //

		// swap for big-endian platform
		pRsnie_cipher->version = cpu2le16(pRsnie_cipher->version);
	    pRsnie_cipher->ucount = cpu2le16(pRsnie_cipher->ucount);
	}
	else
	{
		RSNIE	*pRsnie_cipher = (RSNIE*)pRsnIe;

		// Assign OUI and version
		NdisMoveMemory(pRsnie_cipher->oui, OUI_WPA_VERSION, 4);
        pRsnie_cipher->version = 1;

		switch (WepStatus)
		{
			// TKIP mode
            case Ndis802_11Encryption2Enabled:
                NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA_TKIP, 4);
                pRsnie_cipher->ucount = 1;
                NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA_TKIP, 4);
                *rsn_len = sizeof(RSNIE);
                break;

			// AES mode
            case Ndis802_11Encryption3Enabled:
				if (bMixCipher)
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA_TKIP, 4);
				else
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA_CCMP, 4);
                pRsnie_cipher->ucount = 1;
                NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA_CCMP, 4);
                *rsn_len = sizeof(RSNIE);
                break;

			// TKIP-AES mix mode
            case Ndis802_11Encryption4Enabled:
                NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA_TKIP, 4);

				PairwiseCnt = 1;
				// Insert WPA TKIP as the first pairwise cipher
				if (MIX_CIPHER_WPA_TKIP_ON(FlexibleCipher))
				{
                	NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA_TKIP, 4);
					// Insert WPA AES as the secondary pairwise cipher
					if (MIX_CIPHER_WPA_AES_ON(FlexibleCipher))
					{
                		NdisMoveMemory(pRsnie_cipher->ucast[0].oui + 4, OUI_WPA_CCMP, 4);
						PairwiseCnt = 2;
					}
				}
				else
				{
					// Insert WPA AES as the first pairwise cipher
					NdisMoveMemory(pRsnie_cipher->ucast[0].oui, OUI_WPA_CCMP, 4);
				}

                pRsnie_cipher->ucount = PairwiseCnt;
                *rsn_len = sizeof(RSNIE) + (4 * (PairwiseCnt - 1));
                break;
        }

#ifdef CONFIG_STA_SUPPORT
		if ((pAd->OpMode == OPMODE_STA) &&
			(pAd->StaCfg.GroupCipher != Ndis802_11Encryption2Enabled) &&
			(pAd->StaCfg.GroupCipher != Ndis802_11Encryption3Enabled))
		{
			UINT GroupCipher = pAd->StaCfg.GroupCipher;
			switch(GroupCipher)
			{
				case Ndis802_11GroupWEP40Enabled:
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA_WEP40, 4);
					break;
				case Ndis802_11GroupWEP104Enabled:
					NdisMoveMemory(pRsnie_cipher->mcast, OUI_WPA_WEP104, 4);
					break;
			}
		}
#endif // CONFIG_STA_SUPPORT //

		// swap for big-endian platform
		pRsnie_cipher->version = cpu2le16(pRsnie_cipher->version);
	    pRsnie_cipher->ucount = cpu2le16(pRsnie_cipher->ucount);
	}
}

static VOID RTMPInsertRsnIeAKM(
	IN  PRTMP_ADAPTER   pAd,
	IN	UCHAR			ElementID,
	IN	UINT			AuthMode,
	IN	UCHAR			apidx,
	OUT	PUCHAR			pRsnIe,
	OUT	UCHAR			*rsn_len)
{
	RSNIE_AUTH		*pRsnie_auth;

	pRsnie_auth = (RSNIE_AUTH*)(pRsnIe + (*rsn_len));

	// decide WPA2 or WPA1
	if (ElementID == Wpa2Ie)
	{
		switch (AuthMode)
        {
            case Ndis802_11AuthModeWPA2:
            case Ndis802_11AuthModeWPA1WPA2:
                pRsnie_auth->acount = 1;
                	NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA2_8021X_AKM, 4);
                break;

            case Ndis802_11AuthModeWPA2PSK:
            case Ndis802_11AuthModeWPA1PSKWPA2PSK:
                pRsnie_auth->acount = 1;
                	NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA2_PSK_AKM, 4);
                break;
        }
	}
	else
	{
		switch (AuthMode)
        {
            case Ndis802_11AuthModeWPA:
            case Ndis802_11AuthModeWPA1WPA2:
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_8021X_AKM, 4);
                break;

            case Ndis802_11AuthModeWPAPSK:
            case Ndis802_11AuthModeWPA1PSKWPA2PSK:
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_PSK_AKM, 4);
                break;

			case Ndis802_11AuthModeWPANone:
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_NONE_AKM, 4);
                break;
        }
	}

	pRsnie_auth->acount = cpu2le16(pRsnie_auth->acount);

	(*rsn_len) += sizeof(RSNIE_AUTH);	// update current RSNIE length

}

static VOID RTMPInsertRsnIeCap(
	IN  PRTMP_ADAPTER   pAd,
	IN	UCHAR			ElementID,
	IN	UCHAR			apidx,
	OUT	PUCHAR			pRsnIe,
	OUT	UCHAR			*rsn_len)
{
	RSN_CAPABILITIES    *pRSN_Cap;

	// it could be ignored in WPA1 mode
	if (ElementID == WpaIe)
		return;

	pRSN_Cap = (RSN_CAPABILITIES*)(pRsnIe + (*rsn_len));


	pRSN_Cap->word = cpu2le16(pRSN_Cap->word);

	(*rsn_len) += sizeof(RSN_CAPABILITIES);	// update current RSNIE length

}


VOID RTMPMakeRSNIE(
    IN  PRTMP_ADAPTER   pAd,
    IN  UINT            AuthMode,
    IN  UINT            WepStatus,
	IN	UCHAR			apidx)
{
	PUCHAR		pRsnIe = NULL;			// primary RSNIE
	UCHAR 		*rsnielen_cur_p = 0;	// the length of the primary RSNIE
	UCHAR		*rsnielen_ex_cur_p = 0;	// the length of the secondary RSNIE
	UCHAR		PrimaryRsnie;
	BOOLEAN		bMixCipher = FALSE;	// indicate the pairwise and group cipher are different
	UCHAR		p_offset;
	WPA_MIX_PAIR_CIPHER		FlexibleCipher = MIX_CIPHER_NOTUSE;	// it provide the more flexible cipher combination in WPA-WPA2 and TKIPAES mode

	rsnielen_cur_p = NULL;
	rsnielen_ex_cur_p = NULL;

	{
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
#ifdef WPA_SUPPLICANT_SUPPORT
			if (pAd->StaCfg.WpaSupplicantUP != WPA_SUPPLICANT_DISABLE)
			{
				if (AuthMode < Ndis802_11AuthModeWPA)
					return;
			}
			else
#endif // WPA_SUPPLICANT_SUPPORT //
			{
				// Support WPAPSK or WPA2PSK in STA-Infra mode
				// Support WPANone in STA-Adhoc mode
				if ((AuthMode != Ndis802_11AuthModeWPAPSK) &&
					(AuthMode != Ndis802_11AuthModeWPA2PSK) &&
					(AuthMode != Ndis802_11AuthModeWPANone)
					)
					return;
			}

			DBGPRINT(RT_DEBUG_TRACE,("==> RTMPMakeRSNIE(STA)\n"));

			// Zero RSNIE context
			pAd->StaCfg.RSNIE_Len = 0;
			NdisZeroMemory(pAd->StaCfg.RSN_IE, MAX_LEN_OF_RSNIE);

			// Pointer to RSNIE
			rsnielen_cur_p = &pAd->StaCfg.RSNIE_Len;
			pRsnIe = pAd->StaCfg.RSN_IE;

			bMixCipher = pAd->StaCfg.bMixCipher;
		}
#endif // CONFIG_STA_SUPPORT //
	}

	// indicate primary RSNIE as WPA or WPA2
	if ((AuthMode == Ndis802_11AuthModeWPA) ||
		(AuthMode == Ndis802_11AuthModeWPAPSK) ||
		(AuthMode == Ndis802_11AuthModeWPANone) ||
		(AuthMode == Ndis802_11AuthModeWPA1WPA2) ||
		(AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
		PrimaryRsnie = WpaIe;
	else
		PrimaryRsnie = Wpa2Ie;

	{
		// Build the primary RSNIE
		// 1. insert cipher suite
		RTMPInsertRsnIeCipher(pAd, PrimaryRsnie, WepStatus, bMixCipher, FlexibleCipher, pRsnIe, &p_offset);

		// 2. insert AKM
		RTMPInsertRsnIeAKM(pAd, PrimaryRsnie, AuthMode, apidx, pRsnIe, &p_offset);

		// 3. insert capability
		RTMPInsertRsnIeCap(pAd, PrimaryRsnie, apidx, pRsnIe, &p_offset);
	}

	// 4. update the RSNIE length
	*rsnielen_cur_p = p_offset;

	hex_dump("The primary RSNIE", pRsnIe, (*rsnielen_cur_p));


}

BOOLEAN RTMPCheckWPAframe(
    IN PRTMP_ADAPTER    pAd,
    IN PMAC_TABLE_ENTRY	pEntry,
    IN PUCHAR           pData,
    IN ULONG            DataByteCount,
	IN UCHAR			FromWhichBSSID)
{
	ULONG	Body_len;
	BOOLEAN Cancelled;


    if(DataByteCount < (LENGTH_802_1_H + LENGTH_EAPOL_H))
        return FALSE;


	// Skip LLC header
    if (NdisEqualMemory(SNAP_802_1H, pData, 6) ||
        // Cisco 1200 AP may send packet with SNAP_BRIDGE_TUNNEL
        NdisEqualMemory(SNAP_BRIDGE_TUNNEL, pData, 6))
    {
        pData += 6;
    }
	// Skip 2-bytes EAPoL type
    if (NdisEqualMemory(EAPOL, pData, 2))
    {
        pData += 2;
    }
    else
        return FALSE;

    switch (*(pData+1))
    {
        case EAPPacket:
			Body_len = (*(pData+2)<<8) | (*(pData+3));
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAP-Packet frame, TYPE = 0, Length = %ld\n", Body_len));
            break;
        case EAPOLStart:
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOL-Start frame, TYPE = 1 \n"));
			if (pEntry->EnqueueEapolStartTimerRunning != EAPOL_START_DISABLE)
            {
            	DBGPRINT(RT_DEBUG_TRACE, ("Cancel the EnqueueEapolStartTimerRunning \n"));
                RTMPCancelTimer(&pEntry->EnqueueStartForPSKTimer, &Cancelled);
                pEntry->EnqueueEapolStartTimerRunning = EAPOL_START_DISABLE;
            }
            break;
        case EAPOLLogoff:
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOLLogoff frame, TYPE = 2 \n"));
            break;
        case EAPOLKey:
			Body_len = (*(pData+2)<<8) | (*(pData+3));
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOL-Key frame, TYPE = 3, Length = %ld\n", Body_len));
            break;
        case EAPOLASFAlert:
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOLASFAlert frame, TYPE = 4 \n"));
            break;
        default:
            return FALSE;

    }
    return TRUE;
}


VOID AES_GTK_KEY_WRAP(
    IN UCHAR    *key,
    IN UCHAR    *plaintext,
    IN UCHAR    p_len,
    OUT UCHAR   *ciphertext)
{
    UCHAR       A[8], BIN[16], BOUT[16];
    UCHAR       R[512];
    INT         num_blocks = p_len/8;   // unit:64bits
    INT         i, j;
    aes_context aesctx;
    UCHAR       xor;

    rtmp_aes_set_key(&aesctx, key, 128);

    // Init IA
    for (i = 0; i < 8; i++)
        A[i] = 0xa6;

    //Input plaintext
    for (i = 0; i < num_blocks; i++)
    {
        for (j = 0 ; j < 8; j++)
            R[8 * (i + 1) + j] = plaintext[8 * i + j];
    }

    // Key Mix
    for (j = 0; j < 6; j++)
    {
        for(i = 1; i <= num_blocks; i++)
        {
            //phase 1
            NdisMoveMemory(BIN, A, 8);
            NdisMoveMemory(&BIN[8], &R[8 * i], 8);
            rtmp_aes_encrypt(&aesctx, BIN, BOUT);

            NdisMoveMemory(A, &BOUT[0], 8);
            xor = num_blocks * j + i;
            A[7] = BOUT[7] ^ xor;
            NdisMoveMemory(&R[8 * i], &BOUT[8], 8);
        }
    }

    // Output ciphertext
    NdisMoveMemory(ciphertext, A, 8);

    for (i = 1; i <= num_blocks; i++)
    {
        for (j = 0 ; j < 8; j++)
            ciphertext[8 * i + j] = R[8 * i + j];
    }
}


VOID	AES_GTK_KEY_UNWRAP(
	IN	UCHAR	*key,
	OUT	UCHAR	*plaintext,
	IN	UCHAR    c_len,
	IN	UCHAR	*ciphertext)

{
	UCHAR       A[8], BIN[16], BOUT[16];
	UCHAR       xor;
	INT         i, j;
	aes_context aesctx;
	UCHAR       *R;
	INT         num_blocks = c_len/8;	// unit:64bits


	os_alloc_mem(NULL, (PUCHAR *)&R, 512);

	if (R == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("!!!AES_GTK_KEY_UNWRAP: no memory!!!\n"));
        return;
    } /* End of if */

	// Initialize
	NdisMoveMemory(A, ciphertext, 8);
	//Input plaintext
	for(i = 0; i < (c_len-8); i++)
	{
		R[ i] = ciphertext[i + 8];
	}

	rtmp_aes_set_key(&aesctx, key, 128);

	for(j = 5; j >= 0; j--)
	{
		for(i = (num_blocks-1); i > 0; i--)
		{
			xor = (num_blocks -1 )* j + i;
			NdisMoveMemory(BIN, A, 8);
			BIN[7] = A[7] ^ xor;
			NdisMoveMemory(&BIN[8], &R[(i-1)*8], 8);
			rtmp_aes_decrypt(&aesctx, BIN, BOUT);
			NdisMoveMemory(A, &BOUT[0], 8);
			NdisMoveMemory(&R[(i-1)*8], &BOUT[8], 8);
		}
	}

	// OUTPUT
	for(i = 0; i < c_len; i++)
	{
		plaintext[i] = R[i];
	}


	os_free_mem(NULL, R);
}

CHAR *GetEapolMsgType(CHAR msg)
{
    if(msg == EAPOL_PAIR_MSG_1)
        return "Pairwise Message 1";
    else if(msg == EAPOL_PAIR_MSG_2)
        return "Pairwise Message 2";
	else if(msg == EAPOL_PAIR_MSG_3)
        return "Pairwise Message 3";
	else if(msg == EAPOL_PAIR_MSG_4)
        return "Pairwise Message 4";
	else if(msg == EAPOL_GROUP_MSG_1)
        return "Group Message 1";
	else if(msg == EAPOL_GROUP_MSG_2)
        return "Group Message 2";
    else
    	return "Invalid Message";
}


BOOLEAN RTMPCheckRSNIE(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pData,
	IN  UCHAR           DataLen,
	IN  MAC_TABLE_ENTRY *pEntry,
	OUT	UCHAR			*Offset)
{
	PUCHAR              pVIE;
	UCHAR               len;
	PEID_STRUCT         pEid;
	BOOLEAN				result = FALSE;

	pVIE = pData;
	len	 = DataLen;
	*Offset = 0;

	while (len > sizeof(RSNIE2))
	{
		pEid = (PEID_STRUCT) pVIE;
		// WPA RSN IE
		if ((pEid->Eid == IE_WPA) && (NdisEqualMemory(pEid->Octet, WPA_OUI, 4)))
		{
			if ((pEntry->AuthMode == Ndis802_11AuthModeWPA || pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) &&
				(NdisEqualMemory(pVIE, pEntry->RSN_IE, pEntry->RSNIE_Len)) &&
				(pEntry->RSNIE_Len == (pEid->Len + 2)))
			{
					result = TRUE;
			}

			*Offset += (pEid->Len + 2);
		}
		// WPA2 RSN IE
		else if ((pEid->Eid == IE_RSN) && (NdisEqualMemory(pEid->Octet + 2, RSN_OUI, 3)))
		{
			if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2 || pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK) &&
				(NdisEqualMemory(pVIE, pEntry->RSN_IE, pEntry->RSNIE_Len)) &&
				(pEntry->RSNIE_Len == (pEid->Len + 2))/* ToDo-AlbertY for mesh*/)
			{
					result = TRUE;
			}

			*Offset += (pEid->Len + 2);
		}
		else
		{
			break;
		}

		pVIE += (pEid->Len + 2);
		len  -= (pEid->Len + 2);
	}


	return result;

}


BOOLEAN RTMPParseEapolKeyData(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pKeyData,
	IN  UCHAR           KeyDataLen,
	IN	UCHAR			GroupKeyIndex,
	IN	UCHAR			MsgType,
	IN	BOOLEAN			bWPA2,
	IN  MAC_TABLE_ENTRY *pEntry)
{
    PKDE_ENCAP          pKDE = NULL;
    PUCHAR              pMyKeyData = pKeyData;
    UCHAR               KeyDataLength = KeyDataLen;
    UCHAR               GTKLEN = 0;
	UCHAR				DefaultIdx = 0;
	UCHAR				skip_offset;

	// Verify The RSN IE contained in pairewise_msg_2 && pairewise_msg_3 and skip it
	if (MsgType == EAPOL_PAIR_MSG_2 || MsgType == EAPOL_PAIR_MSG_3)
    {
		// Check RSN IE whether it is WPA2/WPA2PSK
		if (!RTMPCheckRSNIE(pAd, pKeyData, KeyDataLen, pEntry, &skip_offset))
		{
			// send wireless event - for RSN IE different
			if (pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_RSNIE_DIFF_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0);

        	DBGPRINT(RT_DEBUG_ERROR, ("RSN_IE Different in msg %d of 4-way handshake!\n", MsgType));
			hex_dump("Receive RSN_IE ", pKeyData, KeyDataLen);
			hex_dump("Desired RSN_IE ", pEntry->RSN_IE, pEntry->RSNIE_Len);

			return FALSE;
    	}
    	else
		{
			if (bWPA2 && MsgType == EAPOL_PAIR_MSG_3)
			{
				// skip RSN IE
				pMyKeyData += skip_offset;
				KeyDataLength -= skip_offset;
				DBGPRINT(RT_DEBUG_TRACE, ("RTMPParseEapolKeyData ==> WPA2/WPA2PSK RSN IE matched in Msg 3, Length(%d) \n", skip_offset));
			}
			else
				return TRUE;
		}
	}

	DBGPRINT(RT_DEBUG_TRACE,("RTMPParseEapolKeyData ==> KeyDataLength %d without RSN_IE \n", KeyDataLength));

	// Parse EKD format in pairwise_msg_3_WPA2 && group_msg_1_WPA2
	if (bWPA2 && (MsgType == EAPOL_PAIR_MSG_3 || MsgType == EAPOL_GROUP_MSG_1))
	{
		if (KeyDataLength >= 8)	// KDE format exclude GTK length
    	{
        	pKDE = (PKDE_ENCAP) pMyKeyData;


			DefaultIdx = pKDE->GTKEncap.Kid;

			// Sanity check - KED length
			if (KeyDataLength < (pKDE->Len + 2))
    		{
        		DBGPRINT(RT_DEBUG_ERROR, ("ERROR: The len from KDE is too short \n"));
        		return FALSE;
    		}

			// Get GTK length - refer to IEEE 802.11i-2004 p.82
			GTKLEN = pKDE->Len -6;
			if (GTKLEN < LEN_AES_KEY)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("ERROR: GTK Key length is too short (%d) \n", GTKLEN));
        		return FALSE;
			}

    	}
		else
    	{
			DBGPRINT(RT_DEBUG_ERROR, ("ERROR: KDE format length is too short \n"));
	        return FALSE;
    	}

		DBGPRINT(RT_DEBUG_TRACE, ("GTK in KDE format ,DefaultKeyID=%d, KeyLen=%d \n", DefaultIdx, GTKLEN));
		// skip it
		pMyKeyData += 8;
		KeyDataLength -= 8;

	}
	else if (!bWPA2 && MsgType == EAPOL_GROUP_MSG_1)
	{
		DefaultIdx = GroupKeyIndex;
		DBGPRINT(RT_DEBUG_TRACE, ("GTK DefaultKeyID=%d \n", DefaultIdx));
	}

	// Sanity check - shared key index must be 1 ~ 3
	if (DefaultIdx < 1 || DefaultIdx > 3)
    {
     	DBGPRINT(RT_DEBUG_ERROR, ("ERROR: GTK Key index(%d) is invalid in %s %s \n", DefaultIdx, ((bWPA2) ? "WPA2" : "WPA"), GetEapolMsgType(MsgType)));
        return FALSE;
    }


#ifdef CONFIG_STA_SUPPORT
	// Todo
#endif // CONFIG_STA_SUPPORT //

	return TRUE;

}


VOID	ConstructEapolMsg(
	IN 	PRTMP_ADAPTER    	pAd,
    IN 	UCHAR				AuthMode,
    IN 	UCHAR				WepStatus,
    IN 	UCHAR				GroupKeyWepStatus,
    IN 	UCHAR				MsgType,
    IN	UCHAR				DefaultKeyIdx,
    IN 	UCHAR				*ReplayCounter,
	IN 	UCHAR				*KeyNonce,
	IN	UCHAR				*TxRSC,
	IN	UCHAR				*PTK,
	IN	UCHAR				*GTK,
	IN	UCHAR				*RSNIE,
	IN	UCHAR				RSNIE_Len,
    OUT PEAPOL_PACKET       pMsg)
{
	BOOLEAN	bWPA2 = FALSE;

	// Choose WPA2 or not
	if ((AuthMode == Ndis802_11AuthModeWPA2) || (AuthMode == Ndis802_11AuthModeWPA2PSK))
		bWPA2 = TRUE;

    // Init Packet and Fill header
    pMsg->ProVer = EAPOL_VER;
    pMsg->ProType = EAPOLKey;

	// Default 95 bytes, the EAPoL-Key descriptor exclude Key-data field
	pMsg->Body_Len[1] = LEN_EAPOL_KEY_MSG;

	// Fill in EAPoL descriptor
	if (bWPA2)
		pMsg->KeyDesc.Type = WPA2_KEY_DESC;
	else
		pMsg->KeyDesc.Type = WPA1_KEY_DESC;

	// Fill in Key information, refer to IEEE Std 802.11i-2004 page 78
	// When either the pairwise or the group cipher is AES, the DESC_TYPE_AES(2) shall be used.
	pMsg->KeyDesc.KeyInfo.KeyDescVer =
        	(((WepStatus == Ndis802_11Encryption3Enabled) || (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)) ? (DESC_TYPE_AES) : (DESC_TYPE_TKIP));

	// Specify Key Type as Group(0) or Pairwise(1)
	if (MsgType >= EAPOL_GROUP_MSG_1)
		pMsg->KeyDesc.KeyInfo.KeyType = GROUPKEY;
	else
		pMsg->KeyDesc.KeyInfo.KeyType = PAIRWISEKEY;

	// Specify Key Index, only group_msg1_WPA1
	if (!bWPA2 && (MsgType >= EAPOL_GROUP_MSG_1))
		pMsg->KeyDesc.KeyInfo.KeyIndex = DefaultKeyIdx;

	if (MsgType == EAPOL_PAIR_MSG_3)
		pMsg->KeyDesc.KeyInfo.Install = 1;

	if ((MsgType == EAPOL_PAIR_MSG_1) || (MsgType == EAPOL_PAIR_MSG_3) || (MsgType == EAPOL_GROUP_MSG_1))
		pMsg->KeyDesc.KeyInfo.KeyAck = 1;

	if (MsgType != EAPOL_PAIR_MSG_1)
		pMsg->KeyDesc.KeyInfo.KeyMic = 1;

	if ((bWPA2 && (MsgType >= EAPOL_PAIR_MSG_3)) || (!bWPA2 && (MsgType >= EAPOL_GROUP_MSG_1)))
    {
       	pMsg->KeyDesc.KeyInfo.Secure = 1;
    }

	if (bWPA2 && ((MsgType == EAPOL_PAIR_MSG_3) || (MsgType == EAPOL_GROUP_MSG_1)))
    {
        pMsg->KeyDesc.KeyInfo.EKD_DL = 1;
    }

	// key Information element has done.
	*(USHORT *)(&pMsg->KeyDesc.KeyInfo) = cpu2le16(*(USHORT *)(&pMsg->KeyDesc.KeyInfo));

	// Fill in Key Length
#if 0
	if (bWPA2)
	{
		// In WPA2 mode, the field indicates the length of pairwise key cipher,
		// so only pairwise_msg_1 and pairwise_msg_3 need to fill.
		if ((MsgType == EAPOL_PAIR_MSG_1) || (MsgType == EAPOL_PAIR_MSG_3))
			pMsg->KeyDesc.KeyLength[1] = ((WepStatus == Ndis802_11Encryption2Enabled) ? LEN_TKIP_KEY : LEN_AES_KEY);
	}
	else if (!bWPA2)
#endif
	{
		if (MsgType >= EAPOL_GROUP_MSG_1)
		{
			// the length of group key cipher
			pMsg->KeyDesc.KeyLength[1] = ((GroupKeyWepStatus == Ndis802_11Encryption2Enabled) ? TKIP_GTK_LENGTH : LEN_AES_KEY);
		}
		else
		{
			// the length of pairwise key cipher
			pMsg->KeyDesc.KeyLength[1] = ((WepStatus == Ndis802_11Encryption2Enabled) ? LEN_TKIP_KEY : LEN_AES_KEY);
		}
	}

 	// Fill in replay counter
    NdisMoveMemory(pMsg->KeyDesc.ReplayCounter, ReplayCounter, LEN_KEY_DESC_REPLAY);

	// Fill Key Nonce field
	// ANonce : pairwise_msg1 & pairwise_msg3
	// SNonce : pairwise_msg2
	// GNonce : group_msg1_wpa1
	if ((MsgType <= EAPOL_PAIR_MSG_3) || ((!bWPA2 && (MsgType == EAPOL_GROUP_MSG_1))))
    	NdisMoveMemory(pMsg->KeyDesc.KeyNonce, KeyNonce, LEN_KEY_DESC_NONCE);

	// Fill key IV - WPA2 as 0, WPA1 as random
	if (!bWPA2 && (MsgType == EAPOL_GROUP_MSG_1))
	{
		// Suggest IV be random number plus some number,
		NdisMoveMemory(pMsg->KeyDesc.KeyIv, &KeyNonce[16], LEN_KEY_DESC_IV);
        pMsg->KeyDesc.KeyIv[15] += 2;
	}

    // Fill Key RSC field
    // It contains the RSC for the GTK being installed.
	if ((MsgType == EAPOL_PAIR_MSG_3 && bWPA2) || (MsgType == EAPOL_GROUP_MSG_1))
	{
        NdisMoveMemory(pMsg->KeyDesc.KeyRsc, TxRSC, 6);
	}

	// Clear Key MIC field for MIC calculation later
    NdisZeroMemory(pMsg->KeyDesc.KeyMic, LEN_KEY_DESC_MIC);

	ConstructEapolKeyData(pAd,
						  AuthMode,
						  WepStatus,
						  GroupKeyWepStatus,
						  MsgType,
						  DefaultKeyIdx,
						  bWPA2,
						  PTK,
						  GTK,
						  RSNIE,
						  RSNIE_Len,
						  pMsg);

	// Calculate MIC and fill in KeyMic Field except Pairwise Msg 1.
	if (MsgType != EAPOL_PAIR_MSG_1)
	{
		CalculateMIC(pAd, WepStatus, PTK, pMsg);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("===> ConstructEapolMsg for %s %s\n", ((bWPA2) ? "WPA2" : "WPA"), GetEapolMsgType(MsgType)));
	DBGPRINT(RT_DEBUG_TRACE, ("	     Body length = %d \n", pMsg->Body_Len[1]));
	DBGPRINT(RT_DEBUG_TRACE, ("	     Key length  = %d \n", pMsg->KeyDesc.KeyLength[1]));


}

VOID	ConstructEapolKeyData(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			AuthMode,
	IN	UCHAR			WepStatus,
	IN	UCHAR			GroupKeyWepStatus,
	IN 	UCHAR			MsgType,
	IN	UCHAR			DefaultKeyIdx,
	IN	BOOLEAN			bWPA2Capable,
	IN	UCHAR			*PTK,
	IN	UCHAR			*GTK,
	IN	UCHAR			*RSNIE,
	IN	UCHAR			RSNIE_LEN,
	OUT PEAPOL_PACKET   pMsg)
{
	UCHAR		*mpool, *Key_Data, *Rc4GTK;
	UCHAR       ekey[(LEN_KEY_DESC_IV+LEN_EAP_EK)];
	UCHAR		data_offset;


	if (MsgType == EAPOL_PAIR_MSG_1 || MsgType == EAPOL_PAIR_MSG_4 || MsgType == EAPOL_GROUP_MSG_2)
		return;

	// allocate memory pool
	os_alloc_mem(pAd, (PUCHAR *)&mpool, 1500);

    if (mpool == NULL)
		return;

	/* Rc4GTK Len = 512 */
	Rc4GTK = (UCHAR *) ROUND_UP(mpool, 4);
	/* Key_Data Len = 512 */
	Key_Data = (UCHAR *) ROUND_UP(Rc4GTK + 512, 4);

	NdisZeroMemory(Key_Data, 512);
	pMsg->KeyDesc.KeyDataLen[1] = 0;
	data_offset = 0;

	// Encapsulate RSNIE in pairwise_msg2 & pairwise_msg3
	if (RSNIE_LEN && ((MsgType == EAPOL_PAIR_MSG_2) || (MsgType == EAPOL_PAIR_MSG_3)))
	{
		if (bWPA2Capable)
			Key_Data[data_offset + 0] = IE_WPA2;
		else
			Key_Data[data_offset + 0] = IE_WPA;

        Key_Data[data_offset + 1] = RSNIE_LEN;
		NdisMoveMemory(&Key_Data[data_offset + 2], RSNIE, RSNIE_LEN);
		data_offset += (2 + RSNIE_LEN);
	}

	// Encapsulate KDE format in pairwise_msg3_WPA2 & group_msg1_WPA2
	if (bWPA2Capable && ((MsgType == EAPOL_PAIR_MSG_3) || (MsgType == EAPOL_GROUP_MSG_1)))
	{
		// Key Data Encapsulation (KDE) format - 802.11i-2004  Figure-43w and Table-20h
        Key_Data[data_offset + 0] = 0xDD;

		if (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
		{
			Key_Data[data_offset + 1] = 0x16;// 4+2+16(OUI+DataType+DataField)
		}
		else
		{
			Key_Data[data_offset + 1] = 0x26;// 4+2+32(OUI+DataType+DataField)
		}

        Key_Data[data_offset + 2] = 0x00;
        Key_Data[data_offset + 3] = 0x0F;
        Key_Data[data_offset + 4] = 0xAC;
        Key_Data[data_offset + 5] = 0x01;

		// GTK KDE format - 802.11i-2004  Figure-43x
        Key_Data[data_offset + 6] = (DefaultKeyIdx & 0x03);
        Key_Data[data_offset + 7] = 0x00;	// Reserved Byte

		data_offset += 8;
	}


	// Encapsulate GTK and encrypt the key-data field with KEK.
	// Only for pairwise_msg3_WPA2 and group_msg1
	if ((MsgType == EAPOL_PAIR_MSG_3 && bWPA2Capable) || (MsgType == EAPOL_GROUP_MSG_1))
	{
		// Fill in GTK
		if (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
		{
			NdisMoveMemory(&Key_Data[data_offset], GTK, LEN_AES_KEY);
			data_offset += LEN_AES_KEY;
		}
		else
		{
			NdisMoveMemory(&Key_Data[data_offset], GTK, TKIP_GTK_LENGTH);
			data_offset += TKIP_GTK_LENGTH;
		}

		// Still dont know why, but if not append will occur "GTK not include in MSG3"
		// Patch for compatibility between zero config and funk
		if (MsgType == EAPOL_PAIR_MSG_3 && bWPA2Capable)
		{
			if (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
			{
				Key_Data[data_offset + 0] = 0xDD;
				Key_Data[data_offset + 1] = 0;
				data_offset += 2;
			}
			else
			{
				Key_Data[data_offset + 0] = 0xDD;
				Key_Data[data_offset + 1] = 0;
				Key_Data[data_offset + 2] = 0;
				Key_Data[data_offset + 3] = 0;
				Key_Data[data_offset + 4] = 0;
				Key_Data[data_offset + 5] = 0;
				data_offset += 6;
			}
		}

		// Encrypt the data material in key data field
		if (WepStatus == Ndis802_11Encryption3Enabled)
		{
			AES_GTK_KEY_WRAP(&PTK[16], Key_Data, data_offset, Rc4GTK);
            // AES wrap function will grow 8 bytes in length
            data_offset += 8;
		}
		else
		{
			// PREPARE Encrypted  "Key DATA" field.  (Encrypt GTK with RC4, usinf PTK[16]->[31] as Key, IV-field as IV)
			// put TxTsc in Key RSC field
			pAd->PrivateInfo.FCSCRC32 = PPPINITFCS32;   //Init crc32.

			// ekey is the contanetion of IV-field, and PTK[16]->PTK[31]
			NdisMoveMemory(ekey, pMsg->KeyDesc.KeyIv, LEN_KEY_DESC_IV);
			NdisMoveMemory(&ekey[LEN_KEY_DESC_IV], &PTK[16], LEN_EAP_EK);
			ARCFOUR_INIT(&pAd->PrivateInfo.WEPCONTEXT, ekey, sizeof(ekey));  //INIT SBOX, KEYLEN+3(IV)
			pAd->PrivateInfo.FCSCRC32 = RTMP_CALC_FCS32(pAd->PrivateInfo.FCSCRC32, Key_Data, data_offset);
			WPAARCFOUR_ENCRYPT(&pAd->PrivateInfo.WEPCONTEXT, Rc4GTK, Key_Data, data_offset);
		}

		NdisMoveMemory(pMsg->KeyDesc.KeyData, Rc4GTK, data_offset);
	}
	else
	{
		NdisMoveMemory(pMsg->KeyDesc.KeyData, Key_Data, data_offset);
	}

	// set key data length field and total length
	pMsg->KeyDesc.KeyDataLen[1] = data_offset;
    pMsg->Body_Len[1] += data_offset;

	os_free_mem(pAd, mpool);

}

VOID	CalculateMIC(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			PeerWepStatus,
	IN	UCHAR			*PTK,
	OUT PEAPOL_PACKET   pMsg)
{
    UCHAR   *OutBuffer;
	ULONG	FrameLen = 0;
	UCHAR	mic[LEN_KEY_DESC_MIC];
	UCHAR	digest[80];

	// allocate memory for MIC calculation
	os_alloc_mem(pAd, (PUCHAR *)&OutBuffer, 512);

    if (OutBuffer == NULL)
    {
		DBGPRINT(RT_DEBUG_ERROR, ("!!!CalculateMIC: no memory!!!\n"));
		return;
    }

	// make a frame for calculating MIC.
    MakeOutgoingFrame(OutBuffer,            	&FrameLen,
                      pMsg->Body_Len[1] + 4,  	pMsg,
                      END_OF_ARGS);

	NdisZeroMemory(mic, sizeof(mic));

	// Calculate MIC
    if (PeerWepStatus == Ndis802_11Encryption3Enabled)
 	{
		HMAC_SHA1(OutBuffer,  FrameLen, PTK, LEN_EAP_MICK, digest);
		NdisMoveMemory(mic, digest, LEN_KEY_DESC_MIC);
	}
	else
	{
		hmac_md5(PTK,  LEN_EAP_MICK, OutBuffer, FrameLen, mic);
	}

	// store the calculated MIC
	NdisMoveMemory(pMsg->KeyDesc.KeyMic, mic, LEN_KEY_DESC_MIC);

	os_free_mem(pAd, OutBuffer);
}

NDIS_STATUS	RTMPSoftDecryptBroadCastData(
	IN	PRTMP_ADAPTER					pAd,
	IN	RX_BLK							*pRxBlk,
	IN  NDIS_802_11_ENCRYPTION_STATUS 	GroupCipher,
	IN  PCIPHER_KEY						pShard_key)
{
	PRXWI_STRUC			pRxWI = pRxBlk->pRxWI;



	// handle WEP decryption
	if (GroupCipher == Ndis802_11Encryption1Enabled)
    {
		if (RTMPSoftDecryptWEP(pAd, pRxBlk->pData, pRxWI->MPDUtotalByteCount, pShard_key))
		{

			//Minus IV[4] & ICV[4]
			pRxWI->MPDUtotalByteCount -= 8;
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ERROR : Software decrypt WEP data fails.\n"));
			// give up this frame
			return NDIS_STATUS_FAILURE;
		}
	}
	// handle TKIP decryption
	else if (GroupCipher == Ndis802_11Encryption2Enabled)
	{
		if (RTMPSoftDecryptTKIP(pAd, pRxBlk->pData, pRxWI->MPDUtotalByteCount, 0, pShard_key))
		{

			//Minus 8 bytes MIC, 8 bytes IV/EIV, 4 bytes ICV
			pRxWI->MPDUtotalByteCount -= 20;
		}
        else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ERROR : RTMPSoftDecryptTKIP Failed\n"));
			// give up this frame
			return NDIS_STATUS_FAILURE;
        }
	}
	// handle AES decryption
	else if (GroupCipher == Ndis802_11Encryption3Enabled)
	{
		if (RTMPSoftDecryptAES(pAd, pRxBlk->pData, pRxWI->MPDUtotalByteCount , pShard_key))
		{

			//8 bytes MIC, 8 bytes IV/EIV (CCMP Header)
			pRxWI->MPDUtotalByteCount -= 16;
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("ERROR : RTMPSoftDecryptAES Failed\n"));
			// give up this frame
			return NDIS_STATUS_FAILURE;
		}
	}
	else
	{
		// give up this frame
		return NDIS_STATUS_FAILURE;
	}

	return NDIS_STATUS_SUCCESS;

}

