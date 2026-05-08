modded class CarScript
{
    private bool isLocked, isLogged, isSteal;
    private int pinCode;
    private bool prevLockState;
    protected EffectSound signalAlarm;
    private ref array<string> doorSlots;
    private const string DOOR_SLOTS_RELATIVE_PATH = "CfgVehicles %1 GUIInventoryAttachmentsProps Body attachmentSlots";

    void CarScript()
    {
        RegisterNetSyncVariableBool("isLocked");
        RegisterNetSyncVariableBool("isSteal");
        // pinCode НЕ регистрируем для синхронизации в целях безопасности
        prevLockState = IsLocked();
        isLogged = false;
    }

    bool IsLocked()
    {
        return isLocked;
    }

    bool IsSteal()
    {
        return isSteal;
    }

    bool IsLogged()
    {
        return isLogged;
    }

    bool HasCode()
    {
        return pinCode > 999;
    }

    int GetPinCode()
    {
        return pinCode;
    }

    void LogIn()
    {
        isLogged = true;
    }

    bool HasBattery()
    {
        ItemBase battery;
        if ( IsVitalCarBattery() )
            battery = ItemBase.Cast( FindAttachmentBySlotName("CarBattery") );
        else if ( IsVitalTruckBattery() )
            battery = ItemBase.Cast( FindAttachmentBySlotName("TruckBattery") );
        return battery != null;
    }

    override void OnEngineStart()
    {
        super.OnEngineStart();
        if (GetGame().IsClient())
        {
            AssignPlayer();
        }
    }

    void AssignPlayer()
    {
        PlayerBase driver = PlayerBase.Cast(GetGame().GetPlayer());
        if (!driver)
            return;
        if (CrewMemberIndex( driver ) == DayZPlayerConstants.VEHICLESEAT_DRIVER)
            driver.SetCurrentCar(this);
    }

    override bool IsInventoryVisible()
    {
        if ( !super.IsInventoryVisible() )
            return false;
        return AnyDoorOpen();
    }

    private bool AnyDoorOpen()
    {
        if (!doorSlots)
            LoadDoorSlots();
        if (!doorSlots || !doorSlots.Count())
            return true;
        bool anyDoorOpen;
        bool hasAnyDoor;
        CarDoorState state;
        foreach(string doorSlot : doorSlots)
        {
            state = GetCarDoorsState(doorSlot);
            switch (state)
            {
                case CarDoorState.DOORS_OPEN:
                    anyDoorOpen = true;
                    hasAnyDoor = true;
                    break;
                case CarDoorState.DOORS_CLOSED:
                    hasAnyDoor = true;
                    break;
            }
        }
        if (!hasAnyDoor)
            return true;
        return anyDoorOpen;
    }

    private void LoadDoorSlots()
    {
        string doorSlotsPath = string.Format(DOOR_SLOTS_RELATIVE_PATH, GetType());
        if (GetGame().ConfigIsExisting(doorSlotsPath))
        {
            doorSlots = {};
            GetGame().ConfigGetTextArray(doorSlotsPath, doorSlots);
        }
    }

    override bool CanReleaseAttachment( EntityAI attachment )
    {
        if ( !super.CanReleaseAttachment( attachment ) )
            return false;
        if ( IsLocked() )
            return false;
        return true;
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (prevLockState != IsLocked())
        {
            if (!IsLocked())
                PlayOpenLockSound();
            else
                PlayCloseLockSound();
            prevLockState = IsLocked();
        }
        if (isSteal)
        {
            PlayAlarm();
        }
        else if (signalAlarm && signalAlarm.IsSoundPlaying())
        {
            signalAlarm.Stop();
        }
    }

    void PlayOpenLockSound()
    {
        EffectSound sound = SEffectManager.PlaySound("CARLOCK_Open_SoundSet", GetPosition() );
        sound.SetSoundAutodestroy( true );
    }

    void PlayCloseLockSound()
    {
        EffectSound sound = SEffectManager.PlaySound("CARLOCK_Close_SoundSet", GetPosition() );
        sound.SetSoundAutodestroy( true );
    }

    void PlayAlarm()
    {
        if (signalAlarm && signalAlarm.IsSoundPlaying())
            return;
        signalAlarm = SEffectManager.PlaySoundOnObject("CARLOCK_Signal_SoundSet", this );
        signalAlarm.SetSoundAutodestroy( true );
    }

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionInteractWithLockOnCarDoor);
        AddAction(ActionDirectOpenCarLock);
        AddAction(ActionDirectLockCarLock);
        AddAction(ActionAdminShowCarLockCode);
        AddAction(ActionAdminResetCarLockCode);
    }

    // --- SERVER SIDE ---

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ctx.Write(isLocked);
        ctx.Write(pinCode);
    }

    override void OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return;
        if (!ctx.Read(isLocked))
            isLocked = false;
        if (!ctx.Read(pinCode))
            pinCode = 0;

        SetSynchDirty();
    }

    private ref array<PlayerIdentity> loggedPlayers = new array<PlayerIdentity>;

    bool IsPlayerLogged(PlayerIdentity identity)
    {
        return loggedPlayers.Find(identity) != -1;
    }

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (GetGame().IsClient()) return;

        PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
        if (!player) return;

        switch (rpc_type)
        {
            case -3999346: // Set PinCode
            {
                Param1<int> p_set;
                if (!ctx.Read(p_set)) return;
                if (!HasCode())
                {
                    pinCode = p_set.param1;
                    isLocked = true;
                    SetSynchDirty();
                }
                break;
            }
            case -3999347: // Toggle Lock (Remote)
            {
                if (IsPlayerLogged(sender) && HasBattery())
                {
                    isLocked = !isLocked;
                    SetSynchDirty();
                }
                break;
            }
            case -3999348: // Toggle Alarm
            {
                isSteal = !isSteal;
                SetSynchDirty();
                break;
            }
            case -3999349: // Reset Success (not used in current logic but kept for range)
            {
                break;
            }
            case -3999350: // Admin Request Code
            {
                if (player.IsImmobilizerAdmin())
                {
                    RPCSingleParam(-3999351, new Param1<int>(pinCode), true, sender);
                }
                break;
            }
            case -3999352: // Try Login
            {
                Param1<int> p_login;
                if (!ctx.Read(p_login)) return;
                if (p_login.param1 == pinCode)
                {
                    if (loggedPlayers.Find(sender) == -1)
                        loggedPlayers.Insert(sender);

                    RPCSingleParam(-3999353, new Param1<bool>(true), true, sender);
                }
                else
                {
                    RPCSingleParam(-3999353, new Param1<bool>(false), true, sender);
                }
                break;
            }
        }
    }

    void AdminResetCode()
    {
        pinCode = 0;
        isLocked = false;
        isLogged = false;
        isSteal = false;
        SetSynchDirty();
    }
}
