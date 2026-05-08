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
		CarScript car;
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
			car.AdminResetCode();
		}
	}
}
