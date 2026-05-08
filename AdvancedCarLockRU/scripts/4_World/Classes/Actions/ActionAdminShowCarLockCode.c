class ActionAdminShowCarLockCode : ActionCarDoorsOutside
{
	override string GetText()
	{
		return "[A]Узнать пинкод";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		bool actionCondition = super.ActionCondition(player, target, item);
		CarScript car;

		if (!actionCondition)
			return false;
		if (!player)
			return false;
		if (!CastTo(car, target.GetParent()))
			return false;

        if (GetGame().IsServer())
            return car.HasCode();

		if (car.HasCode() && player.IsImmobilizerAdmin())
			return true;
		return false;
	}

	override bool CanBeUsedInVehicle()
	{
		return false;
	}

	override void OnExecuteServer(ActionData action_data)
	{
		CarScript car;
		if (CastTo(car, action_data.m_Target.GetParent()))
		{
			car.RPCSingleParam(-3999350, null, true, action_data.m_Player.GetIdentity());
		}
	}

	override void OnExecuteClient(ActionData action_data)
	{
		// Клиент ждет RPC от сервера с кодом, само действие просто инициирует запрос
	}
}
