
#include "../rt_config.h"

VOID AuthRspStateMachineInit(
    IN PRTMP_ADAPTER pAd,
    IN PSTATE_MACHINE Sm,
    IN STATE_MACHINE_FUNC Trans[])
{
    StateMachineInit(Sm, Trans, MAX_AUTH_RSP_STATE, MAX_AUTH_RSP_MSG, (STATE_MACHINE_FUNC)Drop, AUTH_RSP_IDLE, AUTH_RSP_MACHINE_BASE);

    // column 1
    StateMachineSetAction(Sm, AUTH_RSP_IDLE, MT2_PEER_DEAUTH, (STATE_MACHINE_FUNC)PeerDeauthAction);

    // column 2
    StateMachineSetAction(Sm, AUTH_RSP_WAIT_CHAL, MT2_PEER_DEAUTH, (STATE_MACHINE_FUNC)PeerDeauthAction);

}

VOID PeerAuthSimpleRspGenAndSend(
    IN PRTMP_ADAPTER pAd,
    IN PHEADER_802_11 pHdr80211,
    IN USHORT Alg,
    IN USHORT Seq,
    IN USHORT Reason,
    IN USHORT Status)
{
    HEADER_802_11     AuthHdr;
    ULONG             FrameLen = 0;
    PUCHAR            pOutBuffer = NULL;
    NDIS_STATUS       NStatus;

    if (Reason != MLME_SUCCESS)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("Peer AUTH fail...\n"));
        return;
    }

	//Get an unused nonpaged memory
    NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);
    if (NStatus != NDIS_STATUS_SUCCESS)
        return;

    DBGPRINT(RT_DEBUG_TRACE, ("Send AUTH response (seq#2)...\n"));
    MgtMacHeaderInit(pAd, &AuthHdr, SUBTYPE_AUTH, 0, pHdr80211->Addr2, pAd->MlmeAux.Bssid);
    MakeOutgoingFrame(pOutBuffer,               &FrameLen,
                      sizeof(HEADER_802_11),    &AuthHdr,
                      2,                        &Alg,
                      2,                        &Seq,
                      2,                        &Reason,
                      END_OF_ARGS);
    MiniportMMRequest(pAd, 0, pOutBuffer, FrameLen);
	MlmeFreeMemory(pAd, pOutBuffer);
}

VOID PeerDeauthAction(
    IN PRTMP_ADAPTER pAd,
    IN PMLME_QUEUE_ELEM Elem)
{
    UCHAR       Addr2[MAC_ADDR_LEN];
    USHORT      Reason;

    if (PeerDeauthSanity(pAd, Elem->Msg, Elem->MsgLen, Addr2, &Reason))
    {
        if (INFRA_ON(pAd) && MAC_ADDR_EQUAL(Addr2, pAd->CommonCfg.Bssid))
        {
            DBGPRINT(RT_DEBUG_TRACE,("AUTH_RSP - receive DE-AUTH from our AP (Reason=%d)\n", Reason));


#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
            {
                union iwreq_data    wrqu;
                memset(wrqu.ap_addr.sa_data, 0, MAC_ADDR_LEN);
                wireless_send_event(pAd->net_dev, SIOCGIWAP, &wrqu, NULL);
            }
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //


			// send wireless event - for deauthentication
			if (pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_DEAUTH_EVENT_FLAG, pAd->MacTab.Content[BSSID_WCID].Addr, BSS0, 0);

            LinkDown(pAd, TRUE);

            // Authentication Mode Cisco_LEAP has start a timer
            // We should cancel it if using LEAP
#ifdef LEAP_SUPPORT
            if (pAd->StaCfg.LeapAuthMode == CISCO_AuthModeLEAP)
            {
                RTMPCancelTimer(&pAd->StaCfg.LeapAuthTimer, &TimerCancelled);
                //Check is it mach the LEAP Authentication failed as possible a Rogue AP
                //on it's PortSecured not equal to WPA_802_1X_PORT_SECURED while process the Authenticaton.
                if ((pAd->StaCfg.PortSecured != WPA_802_1X_PORT_SECURED) && (pAd->Mlme.LeapMachine.CurrState != LEAP_IDLE))
                {
                    RogueApTableSetEntry(pAd, &pAd->StaCfg.RogueApTab, Addr2, LEAP_REASON_AUTH_TIMEOUT);
                }
            }
#endif // LEAP_SUPPORT //
        }
    }
    else
    {
        DBGPRINT(RT_DEBUG_TRACE,("AUTH_RSP - PeerDeauthAction() sanity check fail\n"));
    }
}

