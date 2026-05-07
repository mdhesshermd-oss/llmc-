class ActionAdminResetCarLockCode : ActionInteractBase
{
	void ActionAdminResetCarLockCode()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;

		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
	}

	override string GetText()
	{
		return "[A]Сбросить пинкод";
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

	}

	override void OnStart( ActionData action_data )
	{

	}

	override void OnStartServer( ActionData action_data )
	{
		CarScript car = CarScript.Cast(action_data.m_Target.GetParent());
		if (car)
		{
			car.ResetLock();
		}
	}

	override void OnEnd( ActionData action_data )
	{

	}
}