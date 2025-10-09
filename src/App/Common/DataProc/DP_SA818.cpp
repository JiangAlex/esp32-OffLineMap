#include "DataProc.h"
#include "../HAL/HAL.h"

static void onTimer(Account *account)
{
    // 定期發布 SA818 資訊
    HAL::SA818_Info_t info;
    HAL::SA818_GetInfo(&info);
    account->Commit(&info, sizeof(info));
}

static int onEvent(Account *account, Account::EventParam_t *param)
{
    if (param->event == Account::EVENT_TIMER)
    {
        onTimer(account);
        return Account::RES_OK;
    }

    if (param->event != Account::EVENT_SUB_PULL && param->event != Account::EVENT_NOTIFY)
    {
        return Account::RES_UNSUPPORTED_REQUEST;
    }

    if (param->size != sizeof(HAL::SA818_Info_t))
    {
        return Account::RES_SIZE_MISMATCH;
    }

    if (param->event == Account::EVENT_SUB_PULL)
    {
        HAL::SA818_Info_t *info = (HAL::SA818_Info_t *)param->data_p;
        HAL::SA818_GetInfo(info);
        printf("[DP] SA818 PULL\r\n");
    }
    else if (param->event == Account::EVENT_NOTIFY)
    {
        // 處理 SA818 配置更新通知
        printf("[DP] SA818 NOTIFY\r\n");
    }

    return Account::RES_OK;
}

DATA_PROC_INIT_DEF(SA818)
{
    account->SetEventCallback(onEvent);
    
    // 設置定期更新 SA818 資訊 (每5秒)
    account->SetTimerPeriod(5000);
}