

void PMU_PWRKEY_Enable(int reset_sec);
unsigned int PMU_PWRKEY_GetDEBSignal(void);
unsigned int PMU_PWRKEY_GetNonDEBSignal(void);

void PMU_PwrKeyTest_LongPressThreeSeconds(void);
void PMU_PwrKeyTest_LongPressFiveSeconds(void);
void PMU_PwrKeyTest_LongPressEightSeconds(void);
void PMU_PwrKeyTest_LongPressTenSeconds(void);
void PMU_CheckDEBPwrKeySingal(void);

