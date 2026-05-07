modded class CarScript
{
	private bool isLocked, isLogged, isSteal;
	private bool m_HasPinCode;
	private int pinCode;

	private bool prevLockState;

	protected EffectSound signalAlarm;

	private bool m_HeadlightsState;

	private ref array<string> doorSlots;

	private const string DOOR_SLOTS_RELATIVE_PATH = "CfgVehicles %1 GUIInventoryAttachmentsProps Body attachmentSlots";

	void CarScript()
	{
		RegisterNetSyncVariableBool("isLocked");
		RegisterNetSyncVariableBool("isSteal");
		RegisterNetSyncVariableBool("m_HasPinCode");
		// pinCode is NOT synced for security

		prevLockState = isLocked;

		isLogged = false;
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(isLocked);
		ctx.Write(pinCode);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;

		if (!ctx.Read(isLocked))
			return false;

		if (!ctx.Read(pinCode))
			return false;

		m_HasPinCode = (pinCode > 999);

		return true;
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (GetGame().IsClient()) return;

		PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
		if (!player) return;

		switch (rpc_type)
		{
			case -3999346: // Set Code
			{
				Param1<int> data;
				if (!ctx.Read(data)) return;
				pinCode = data.param1;
				m_HasPinCode = (pinCode > 999);
				isLocked = true;
				SetSynchDirty();
				break;
			}
			case -3999347: // Toggle Lock (Remote)
			{
				isLocked = !isLocked;
				SetSynchDirty();
				break;
			}
			case -3999350: // Admin Request Pin
			{
				if (player.IsImmobilizerAdmin())
				{
					player.RPCSingleParam(-3999351, new Param1<int>(pinCode), true, sender);
				}
				break;
			}
			case -3999353: // Verify Pin (from UI)
			{
				Param1<int> pinData;
				if (!ctx.Read(pinData)) return;

				bool success = (pinData.param1 == pinCode);
				player.RPCSingleParam(-3999354, new Param1<bool>(success), true, sender);
				break;
			}
		}
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
		if (GetGame().IsClient())
			return m_HasPinCode;

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

	void SetAlarmState(bool state)
	{
		if (GetGame().IsServer())
		{
			isSteal = state;
			SetSynchDirty();
		}
	}

	void ResetLock()
	{
		if (GetGame().IsServer())
		{
			isLocked = false;
			pinCode = 0;
			m_HasPinCode = false;
			isLogged = false;
			isSteal = false;
			SetSynchDirty();
		}
	}

	void ToggleLock()
	{
		if (GetGame().IsServer())
		{
			isLocked = !isLocked;
			SetSynchDirty();
		}
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
			StartFlashingLights();
		}
		else
		{
			StopAlarm();
			StopFlashingLights();
		}
	}

	void StartFlashingLights()
	{
		if (GetGame().IsServer()) return;
		if (!GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).IsRunning(this, "ToggleHeadlights"))
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ToggleHeadlights, 500, true);
		}
	}

	void StopFlashingLights()
	{
		if (GetGame().IsServer()) return;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(this, "ToggleHeadlights");
		SetHeadlightsState(false);
	}

	void ToggleHeadlights()
	{
		m_HeadlightsState = !m_HeadlightsState;
		SetHeadlightsState(m_HeadlightsState);
	}

	void SetHeadlightsState(bool state)
	{
		if (state)
		{
			ForceFarLightOn();
		}
		else
		{
			ForceFarLightOff();
		}
	}

	void PlayOpenLockSound()
	{
		EffectSound sound =	SEffectManager.PlaySound("CARLOCK_Open_SoundSet", GetPosition() );
		sound.SetSoundAutodestroy( true );
	}

	void PlayCloseLockSound()
	{
		EffectSound sound =	SEffectManager.PlaySound("CARLOCK_Close_SoundSet", GetPosition() );
		sound.SetSoundAutodestroy( true );
	}

	void PlayAlarm()
	{
		if (signalAlarm && signalAlarm.IsSoundPlaying())
			return;

		signalAlarm = SEffectManager.PlaySoundOnObject("CARLOCK_Signal_SoundSet", this );
		if (signalAlarm)
			signalAlarm.SetSoundAutodestroy( true );
	}

	void StopAlarm()
	{
		if (GetGame().IsServer())
		{
			isSteal = false;
			SetSynchDirty();
		}

		if (signalAlarm && signalAlarm.IsSoundPlaying())
			signalAlarm.Stop();
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
}