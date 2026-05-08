modded class PlayerBase
{
	const private int SIGNAL_COOLDOWN = 3; // sec
	const private int MAX_DISTANCE_REMOTE_OPEN = 5; // meters

	private CarScript currentActiveCar;
	private int carLow, carHigh;
	private bool canDstOpenCar;

	private string carUnlockToolType;
	private float carUnlockDuration;

	private bool ImmobilizerAdmin;

	private static int IMMOBILIZER_SYNCH = -999997865;

	override void Init()
	{
		super.Init();

		canDstOpenCar = true;
	}

	void SetCurrentCar(CarScript car)
	{
		currentActiveCar = car;
		car.GetNetworkID(carLow, carHigh);
	}

	CarScript GetCurrentCar()
	{
		return currentActiveCar;
	}

	bool IsImmobilizerAdmin()
	{
		return ImmobilizerAdmin;
	}

	void TryOpenCarRemote()
	{
		if (GetGame().IsServer())
			return;
		if (!currentActiveCar)
			currentActiveCar = CarScript.Cast(GetGame().GetObjectByNetworkId(carLow, carHigh));
		if (!currentActiveCar)
			return;
		if (currentActiveCar.IsMoving())
			return;
		if (!currentActiveCar.HasBattery())
			return;
		if (currentActiveCar.HasCode() && !currentActiveCar.IsLogged())
			return;
		if (!currentActiveCar.IsLogged())
			return;
		if (!canDstOpenCar)
			return;
		if (vector.Distance(currentActiveCar.GetPosition(), GetPosition()) > MAX_DISTANCE_REMOTE_OPEN)
			return;

		OpenCar();
	}

	void OpenCar()
	{
		currentActiveCar.RPCSingleParam(-3999347, null, true);
		OnOpenCar();
	}

	void OnOpenCar()
	{
		OpenCarCooldownStart();
	}

	void OpenCarCooldownStart()
	{
		canDstOpenCar = false;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OpenCarCooldownFinish, SIGNAL_COOLDOWN * 1000, false);
	}

	void OpenCarCooldownFinish()
	{
		canDstOpenCar = true;
	}

	float GetUnlockDuration()
	{
		return carUnlockDuration;
	}

	bool HasSuitableToolForUnlock()
	{
		EntityAI item = GetItemInHands();

		if (!item)
			return false;

		return item.IsKindOf(carUnlockToolType);
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (GetGame().IsClient())
		{
			if (rpc_type == IMMOBILIZER_SYNCH)
			{
				Param3<bool, string, float> data;
				if (!ctx.Read(data))
					return;

				ImmobilizerAdmin = data.param1;
				carUnlockToolType = data.param2;
				carUnlockDuration = data.param3;
			}
			else if (rpc_type == -3999351)
			{
				Param1<int> p_code;
				if (!ctx.Read(p_code)) return;
				GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Пароль автомобиля: " + p_code.param1, ""));
			}
			else if (rpc_type == -3999353)
			{
				Param1<bool> p_success;
				if (!ctx.Read(p_success)) return;

				if (p_success.param1)
				{
					if (currentActiveCar)
						currentActiveCar.LogIn();
					GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Доступ разрешен", ""));
				}
				else
				{
					GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Доступ запрещен", ""));
				}
			}
		}
	}
}