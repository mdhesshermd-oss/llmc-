class ActionDestroyCarLockCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		float time;

		if (GetGame().IsClient())
		{
			time = m_ActionData.m_Player.GetUnlockDuration();
			m_ActionData.m_ActionComponent = new CAContinuousTime( time );
		}
        else
        {
            // На сервере тоже нужно задать время для корректной работы
            m_ActionData.m_ActionComponent = new CAContinuousTime( 60.0 ); // Будет переопределено если нужно, но база должна быть
        }
	}
};

class ActionDestroyCarLock : ActionContinuousBase
{
	void ActionDestroyCarLock()
	{
		m_CallbackClass = ActionDestroyCarLockCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;

		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
	}

	override string GetText()
	{
		return "Сломать замок";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined;
		m_ConditionTarget = new CCTNone;
	}

	override bool ActionCondition ( PlayerBase player, ActionTarget target, ItemBase item )
	{
		CarScript car;
		if ( !IsInReach(player, target, UAMaxDistances.DEFAULT) )
			return false;

		if ( Class.CastTo(car, target.GetParent()) )
		{
            if (GetGame().IsServer())
                return (car.HasCode() && !car.IsSteal());

			return (car.HasCode() && !car.IsLogged() && !car.IsSteal() && player.HasSuitableToolForUnlock());
		}

		return false;
	}

	override void OnFinishServer( ActionData action_data )
	{
		CarScript car;
		if ( Class.CastTo(car, action_data.m_Target.GetParent()) )
		{
			car.AdminResetCode();

            if (action_data.m_MainItem)
                action_data.m_MainItem.SetHealth(0); // Ломаем отмычку
		}
	}
}
