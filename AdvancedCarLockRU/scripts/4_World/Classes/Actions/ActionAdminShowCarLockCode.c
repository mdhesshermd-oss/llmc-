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
		if (car.HasCode() && player.IsImmobilizerAdmin())
			return true;
		return false;
	}

	override bool CanBeUsedInVehicle()
	{
		return false;
	}

	override void OnExecuteClient(ActionData action_data)
	{
		CarScript car;
		if (CastTo(car, action_data.m_Target.GetParent()))
		{
			car.RPCSingleParam(-3999350, null, true);
		}
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