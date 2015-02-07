

#ifndef _EPL_DEF_H_
#define _EPL_DEF_H_

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#define EPL_C_ADR_BROADCAST         0xFF	// EPL broadcast address
#define EPL_C_ADR_DIAG_DEF_NODE_ID  0xFD	// EPL default address of dignostic device
#define EPL_C_ADR_DUMMY_NODE_ID     0xFC	// EPL dummy node address
#define EPL_C_ADR_INVALID           0x00	// invalid EPL address
#define EPL_C_ADR_MN_DEF_NODE_ID    0xF0	// EPL default address of MN
#define EPL_C_ADR_RT1_DEF_NODE_ID   0xFE	// EPL default address of router type 1
#define EPL_C_DLL_ASND_PRIO_NMTRQST 7	// increased ASnd request priority to be used by NMT Requests
#define EPL_C_DLL_ASND_PRIO_STD     0	// standard ASnd request priority
#define EPL_C_DLL_ETHERTYPE_EPL     0x88AB
#define EPL_C_DLL_ISOCHR_MAX_PAYL   1490	// Byte: maximum size of PReq and PRes payload data, requires C_IP_MAX_MTU
#define EPL_C_DLL_MAX_ASYNC_MTU     1500	// Byte: maximum asynchronous payload in bytes
#define EPL_C_DLL_MAX_PAYL_OFFSET   1499	// Byte: maximum offset of Ethernet frame payload, requires C_IP_MAX_MTU
#define EPL_C_DLL_MAX_RS            7
#define EPL_C_DLL_MIN_ASYNC_MTU     282	// Byte: minimum asynchronous payload in bytes.
#define EPL_C_DLL_MIN_PAYL_OFFSET   45	// Byte: minimum offset of Ethernet frame payload
#define EPL_C_DLL_MULTICAST_ASND    0x01111E000004LL	// EPL ASnd multicast MAC address, canonical form
#define EPL_C_DLL_MULTICAST_PRES    0x01111E000002LL	// EPL PRes multicast MAC address, canonical form
#define EPL_C_DLL_MULTICAST_SOA     0x01111E000003LL	// EPL SoA multicast MAC address, canonical form
#define EPL_C_DLL_MULTICAST_SOC     0x01111E000001LL	// EPL Soc multicast MAC address, canonical form
#define EPL_C_DLL_PREOP1_START_CYCLES 10	// number of unassigning SoA frames at start of NMT_MS_PRE_OPERATIONAL_1
#define EPL_C_DLL_T_BITTIME         10	// ns: Transmission time per bit on 100 Mbit/s network
#define EPL_C_DLL_T_EPL_PDO_HEADER  10	// Byte: size of PReq and PRes EPL PDO message header
#define EPL_C_DLL_T_ETH2_WRAPPER    18	// Byte: size of Ethernet type II wrapper consisting of header and checksum
#define EPL_C_DLL_T_IFG             640	// ns: Ethernet Interframe Gap
#define EPL_C_DLL_T_MIN_FRAME       5120	// ns: Size of minimum Ethernet frame (without preamble)
#define EPL_C_DLL_T_PREAMBLE        960	// ns: Size of Ethernet frame preamble

#define EPL_C_DLL_MINSIZE_SOC       36	// minimum size of SoC without padding and CRC
#define EPL_C_DLL_MINSIZE_PREQ      60	// minimum size of PRec without CRC
#define EPL_C_DLL_MINSIZE_PRES      60	// minimum size of PRes without CRC
#define EPL_C_DLL_MINSIZE_SOA       24	// minimum size of SoA without padding and CRC
#define EPL_C_DLL_MINSIZE_IDENTRES  176	// minimum size of IdentResponse without CRC
#define EPL_C_DLL_MINSIZE_STATUSRES 72	// minimum size of StatusResponse without CRC
#define EPL_C_DLL_MINSIZE_NMTCMD    20	// minimum size of NmtCommand without CommandData, padding and CRC
#define EPL_C_DLL_MINSIZE_NMTCMDEXT 52	// minimum size of NmtCommand without padding and CRC
#define EPL_C_DLL_MINSIZE_NMTREQ    20	// minimum size of NmtRequest without CommandData, padding and CRC
#define EPL_C_DLL_MINSIZE_NMTREQEXT 52	// minimum size of NmtRequest without padding and CRC

#define EPL_C_ERR_MONITOR_DELAY     10	// Error monitoring start delay (not used in DS 1.0.0)
#define EPL_C_IP_ADR_INVALID        0x00000000L	// invalid IP address (0.0.0.0) used to indicate no change
#define EPL_C_IP_INVALID_MTU        0	// Byte: invalid MTU size used to indicate no change
#define EPL_C_IP_MAX_MTU            1518	// Byte: maximum size in bytes of the IP stack which must be processed.
#define EPL_C_IP_MIN_MTU            300	// Byte: minimum size in bytes of the IP stack which must be processed.
#define EPL_C_NMT_STATE_TOLERANCE   5	// Cycles: maximum reaction time to NMT state commands
#define EPL_C_NMT_STATREQ_CYCLE     5	// sec: StatusRequest cycle time to be applied to AsyncOnly CNs
#define EPL_C_SDO_EPL_PORT          3819

#define EPL_C_DLL_MAX_ASND_SERVICE_IDS  5	// see tEplDllAsndServiceId in EplDll.h

// Default configuration
// ======================

#ifndef EPL_D_PDO_Granularity_U8
#define EPL_D_PDO_Granularity_U8    8	// minimum size of objects to be mapped in bits UNSIGNED8 O O 1 1
#endif

#ifndef EPL_NMT_MAX_NODE_ID
#define EPL_NMT_MAX_NODE_ID         254	// maximum node-ID
#endif

#ifndef EPL_D_NMT_MaxCNNumber_U8
#define EPL_D_NMT_MaxCNNumber_U8    239	// maximum number of supported regular CNs in the Node ID range 1 .. 239 UNSIGNED8 O O 239 239
#endif

// defines for EPL API layer static process image
#ifndef EPL_API_PROCESS_IMAGE_SIZE_IN
#define EPL_API_PROCESS_IMAGE_SIZE_IN   0
#endif

#ifndef EPL_API_PROCESS_IMAGE_SIZE_OUT
#define EPL_API_PROCESS_IMAGE_SIZE_OUT  0
#endif

// configure whether OD access events shall be forwarded
// to user callback function.
// Because of reentrancy for local OD accesses, this has to be disabled
// when application resides in other address space as the stack (e.g. if
// EplApiLinuxUser.c and EplApiLinuxKernel.c are used)
#ifndef EPL_API_OBD_FORWARD_EVENT
#define EPL_API_OBD_FORWARD_EVENT       TRUE
#endif

#ifndef EPL_OBD_MAX_STRING_SIZE
#define EPL_OBD_MAX_STRING_SIZE        32	// is used for objects 0x1008/0x1009/0x100A
#endif

#ifndef EPL_OBD_USE_STORE_RESTORE
#define EPL_OBD_USE_STORE_RESTORE       FALSE
#endif

#ifndef EPL_OBD_CHECK_OBJECT_RANGE
#define EPL_OBD_CHECK_OBJECT_RANGE      TRUE
#endif

#ifndef EPL_OBD_USE_STRING_DOMAIN_IN_RAM
#define EPL_OBD_USE_STRING_DOMAIN_IN_RAM    TRUE
#endif

#ifndef EPL_OBD_USE_VARIABLE_SUBINDEX_TAB
#define EPL_OBD_USE_VARIABLE_SUBINDEX_TAB   TRUE
#endif

#ifndef EPL_OBD_USE_KERNEL
#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDU)) == 0)
#define EPL_OBD_USE_KERNEL                  TRUE
#else
#define EPL_OBD_USE_KERNEL                  FALSE
#endif
#endif

#ifndef EPL_OBD_INCLUDE_A000_TO_DEVICE_PART
#define EPL_OBD_INCLUDE_A000_TO_DEVICE_PART FALSE
#endif

#ifndef EPL_VETH_NAME
#define EPL_VETH_NAME       "epl"	// name of net device in Linux
#endif


// Emergency error codes
// ======================
#define EPL_E_NO_ERROR                  0x0000
// 0xFxxx manufacturer specific error codes
#define EPL_E_NMT_NO_IDENT_RES          0xF001
#define EPL_E_NMT_NO_STATUS_RES         0xF002

// 0x816x HW errors
#define EPL_E_DLL_BAD_PHYS_MODE         0x8161
#define EPL_E_DLL_COLLISION             0x8162
#define EPL_E_DLL_COLLISION_TH          0x8163
#define EPL_E_DLL_CRC_TH                0x8164
#define EPL_E_DLL_LOSS_OF_LINK          0x8165
#define EPL_E_DLL_MAC_BUFFER            0x8166
// 0x82xx Protocol errors
#define EPL_E_DLL_ADDRESS_CONFLICT      0x8201
#define EPL_E_DLL_MULTIPLE_MN           0x8202
// 0x821x Frame size errors
#define EPL_E_PDO_SHORT_RX              0x8210
#define EPL_E_PDO_MAP_VERS              0x8211
#define EPL_E_NMT_ASND_MTU_DIF          0x8212
#define EPL_E_NMT_ASND_MTU_LIM          0x8213
#define EPL_E_NMT_ASND_TX_LIM           0x8214
// 0x823x Timing errors
#define EPL_E_NMT_CYCLE_LEN             0x8231
#define EPL_E_DLL_CYCLE_EXCEED          0x8232
#define EPL_E_DLL_CYCLE_EXCEED_TH       0x8233
#define EPL_E_NMT_IDLE_LIM              0x8234
#define EPL_E_DLL_JITTER_TH             0x8235
#define EPL_E_DLL_LATE_PRES_TH          0x8236
#define EPL_E_NMT_PREQ_CN               0x8237
#define EPL_E_NMT_PREQ_LIM              0x8238
#define EPL_E_NMT_PRES_CN               0x8239
#define EPL_E_NMT_PRES_RX_LIM           0x823A
#define EPL_E_NMT_PRES_TX_LIM           0x823B
// 0x824x Frame errors
#define EPL_E_DLL_INVALID_FORMAT        0x8241
#define EPL_E_DLL_LOSS_PREQ_TH          0x8242
#define EPL_E_DLL_LOSS_PRES_TH          0x8243
#define EPL_E_DLL_LOSS_SOA_TH           0x8244
#define EPL_E_DLL_LOSS_SOC_TH           0x8245
// 0x84xx BootUp Errors
#define EPL_E_NMT_BA1                   0x8410	// other MN in MsNotActive active
#define EPL_E_NMT_BA1_NO_MN_SUPPORT     0x8411	// MN is not supported
#define EPL_E_NMT_BPO1                  0x8420	// mandatory CN was not found or failed in BootStep1
#define EPL_E_NMT_BPO1_GET_IDENT        0x8421	// IdentRes was not received
#define EPL_E_NMT_BPO1_DEVICE_TYPE      0x8422	// wrong device type
#define EPL_E_NMT_BPO1_VENDOR_ID        0x8423	// wrong vendor ID
#define EPL_E_NMT_BPO1_PRODUCT_CODE     0x8424	// wrong product code
#define EPL_E_NMT_BPO1_REVISION_NO      0x8425	// wrong revision number
#define EPL_E_NMT_BPO1_SERIAL_NO        0x8426	// wrong serial number
#define EPL_E_NMT_BPO1_CF_VERIFY        0x8428	// verification of configuration failed
#define EPL_E_NMT_BPO2                  0x8430	// mandatory CN failed in BootStep2
#define EPL_E_NMT_BRO                   0x8440	// CheckCommunication failed for mandatory CN
#define EPL_E_NMT_WRONG_STATE           0x8480	// mandatory CN has wrong NMT state

// Defines for object 0x1F80 NMT_StartUp_U32
// ==========================================
#define EPL_NMTST_STARTALLNODES         0x00000002L	// Bit 1
#define EPL_NMTST_NO_AUTOSTART          0x00000004L	// Bit 2
#define EPL_NMTST_NO_STARTNODE          0x00000008L	// Bit 3
#define EPL_NMTST_RESETALL_MAND_CN      0x00000010L	// Bit 4
#define EPL_NMTST_STOPALL_MAND_CN       0x00000040L	// Bit 6
#define EPL_NMTST_NO_AUTOPREOP2         0x00000080L	// Bit 7
#define EPL_NMTST_NO_AUTOREADYTOOP      0x00000100L	// Bit 8
#define EPL_NMTST_EXT_CNIDENTCHECK      0x00000200L	// Bit 9
#define EPL_NMTST_SWVERSIONCHECK        0x00000400L	// Bit 10
#define EPL_NMTST_CONFCHECK             0x00000800L	// Bit 11
#define EPL_NMTST_NO_RETURN_PREOP1      0x00001000L	// Bit 12
#define EPL_NMTST_BASICETHERNET         0x00002000L	// Bit 13

// Defines for object 0x1F81 NMT_NodeAssignment_AU32
// ==================================================
#define EPL_NODEASSIGN_NODE_EXISTS      0x00000001L	// Bit 0
#define EPL_NODEASSIGN_NODE_IS_CN       0x00000002L	// Bit 1
#define EPL_NODEASSIGN_START_CN         0x00000004L	// Bit 2
#define EPL_NODEASSIGN_MANDATORY_CN     0x00000008L	// Bit 3
#define EPL_NODEASSIGN_KEEPALIVE        0x00000010L	//currently not used in EPL V2 standard
#define EPL_NODEASSIGN_SWVERSIONCHECK   0x00000020L	// Bit 5
#define EPL_NODEASSIGN_SWUPDATE         0x00000040L	// Bit 6
#define EPL_NODEASSIGN_ASYNCONLY_NODE   0x00000100L	// Bit 8
#define EPL_NODEASSIGN_MULTIPLEXED_CN   0x00000200L	// Bit 9
#define EPL_NODEASSIGN_RT1              0x00000400L	// Bit 10
#define EPL_NODEASSIGN_RT2              0x00000800L	// Bit 11
#define EPL_NODEASSIGN_MN_PRES          0x00001000L	// Bit 12
#define EPL_NODEASSIGN_VALID            0x80000000L	// Bit 31

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#endif // #ifndef _EPL_DEF_H_
