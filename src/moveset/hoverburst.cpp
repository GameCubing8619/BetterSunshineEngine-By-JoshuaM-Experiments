#include <SMS/actor/Mario.hxx>
#include <SMS/nozzle/Watergun.hxx>
#include <SMS/raw_fn.hxx>
#include <SMS/sound/MSound.hxx>
#include <SMS/sound/MSoundSESystem.hxx>

#include "module.hxx"

#if BETTER_SMS_HOVER_BURST

using namespace BetterSMS;

static bool sIsTriggerNozzleDead = false;

static void snapToReadyForBurst() {
    TWaterGun *fludd;
    SMS_FROM_GPR(30, fludd);

    if (fludd->mCurrentNozzle == TWaterGun::NozzleType::Hover) {
        ((float *)(fludd))[0x1CEC / 4] = 0.0f;
    } else {
        ((float *)(fludd))[0x1CEC / 4] -= 0.1f;
        if (((float *)(fludd))[0x1CEC / 4] < 0.0f)
            ((float *)(fludd))[0x1CEC / 4] = 0.0f;
    }
}
SMS_PATCH_BL(SMS_PORT_REGION(0x802699CC, 0, 0, 0), snapToReadyForBurst);

static void checkExecWaterGun(TWaterGun *fludd) {
    if (fludd->mCurrentNozzle == TWaterGun::NozzleType::Spray ||
        fludd->mCurrentNozzle == TWaterGun::NozzleType::Yoshi ||
        fludd->mCurrentNozzle == TWaterGun::NozzleType::Underwater) {
        emit__9TWaterGunFv(fludd);
        return;
    }

    if (!sIsTriggerNozzleDead)
        emit__9TWaterGunFv(fludd);
}
SMS_PATCH_BL(SMS_PORT_REGION(0x8024E548, 0x802462D4, 0, 0), checkExecWaterGun);

static void killTriggerNozzle() {
    TNozzleTrigger *nozzle;
    SMS_FROM_GPR(29, nozzle);

    nozzle->mSprayState = TNozzleTrigger::DEAD;
    if (nozzle->mFludd->mCurrentNozzle == TWaterGun::NozzleType::Hover ||
        nozzle->mFludd->mCurrentNozzle == TWaterGun::NozzleType::Rocket ||
        nozzle->mFludd->mCurrentNozzle == TWaterGun::NozzleType::Turbo)
        sIsTriggerNozzleDead = true;
}
SMS_PATCH_BL(SMS_PORT_REGION(0x8026C370, 0x802640FC, 0, 0), killTriggerNozzle);

// 0x80262580
// extern -> SME.cpp
static bool checkAirNozzle() {
    TMario *player;
    SMS_FROM_GPR(31, player);

    sIsTriggerNozzleDead &= (player->mState & static_cast<u32>(TMario::STATE_AIRBORN)) != 0;

    if (player->mFludd->mCurrentNozzle == TWaterGun::Spray ||
        player->mFludd->mCurrentNozzle == TWaterGun::Yoshi ||
        player->mFludd->mCurrentNozzle == TWaterGun::Underwater)
        return player->mState != static_cast<u32>(TMario::STATE_HOVER_F);

    return (!(player->mState & static_cast<u32>(TMario::STATE_AIRBORN)) || !sIsTriggerNozzleDead);
}
SMS_PATCH_BL(SMS_PORT_REGION(0x80262580, 0x8025A30C, 0, 0), checkAirNozzle);
SMS_WRITE_32(SMS_PORT_REGION(0x80262584, 0x8025A310, 0, 0), 0x2C030000);

// extern -> fluddgeneral.cpp
void checkSpamHover(TNozzleTrigger *nozzle, u32 r4, TWaterEmitInfo *emitInfo) {
    TMario *player = nozzle->mFludd->mMario;
    Vec size;
    player->JSGGetScaling(&size);

    if (nozzle->mFludd->mCurrentNozzle != TWaterGun::Hover)
        return;

    nozzle->mEmitPerFrame = 1.0f;
    nozzle->mEmitRandom   = 0.0f;

    if (!(player->mControllerWork->mAnalogR < 0.9f) ||
        !(player->mControllerWork->mFrameInput & TMarioControllerWork::A))
        return;

    if ((nozzle->mMaxSprayQuarterFrames - nozzle->mSprayQuarterFramesLeft) >= 20)
        return;

    if (nozzle->mFludd->mCurrentWater < 510)
        return;

    nozzle->mEmitPerFrame           = 255.0f;
    nozzle->mEmitRandom             = 0.5f;
    nozzle->mSprayQuarterFramesLeft = 0;
    nozzle->mSprayState             = TNozzleTrigger::DEAD;

    nozzle->mFludd->mCurrentWater -= 255;
    player->mSpeed.y += (70.0f * size.y) - player->mSpeed.y;
    player->mJumpingState &= 0xFFFFFEFF;

    sIsTriggerNozzleDead = true;
    return;
}

#else

// extern -> fluddgeneral.cpp
void checkSpamHover(TNozzleTrigger *nozzle, u32 r4, TWaterEmitInfo *emitInfo) {}

#endif