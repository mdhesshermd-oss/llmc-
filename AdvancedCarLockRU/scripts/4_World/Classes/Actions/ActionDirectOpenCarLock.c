class ActionDirectOpenCarLock : ActionCarDoorsOutside
{
	override string GetText()
	{
		return "Разблокировать";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		bool actionCondition = super.ActionCondition(player, target, item);
		CarScript car;

		if (GetGame().IsServer())
			return true;
		if (!actionCondition)
			return false;
		if (!player)
			return false;
		if (!CastTo(car, target.GetParent()))
			return false;
		if (car.HasCode() && car.IsLogged() && car.IsLocked())
			return true;
		return false;
	}

	override bool CanBeUsedInVehicle()
	{
		return false;
	}

	override void OnExecuteClient(ActionData action_data)
	{
		action_data.m_Player.SetCurrentCar(action_data.m_Target.GetParent());
	}

	override void OnStart( ActionData action_data )
	{

	}

	override void OnStartServer( ActionData action_data )
	{

	}

	override void OnEnd( ActionData action_data )
	{

	}
}