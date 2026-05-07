class ActionDestroyCarLockCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		float time = 60.0;
		if (m_ActionData.m_Player)
			time = m_ActionData.m_Player.GetUnlockDuration();

		if (time <= 0) time = 60.0;

		m_ActionData.m_ActionComponent = new CAContinuousTime( time );
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
		return "Вскрыть замок отмычкой";
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
			return (car.HasCode() && !car.IsLogged() && player.HasSuitableToolForUnlock());
		}

		return false;
	}

	override void OnStartServer( ActionData action_data )
	{
		CarScript car = CarScript.Cast(action_data.m_Target.GetParent());
		if (car)
		{
			car.RPCSingleParam(-3999348, new Param1<bool>(true), true); // Start Alarm
		}
	}

	override void OnFinishProgressServer( ActionData action_data )
	{
		CarScript car = CarScript.Cast(action_data.m_Target.GetParent());
		if (car)
		{
			car.RPCSingleParam(-3999348, new Param1<bool>(false), true); // Stop Alarm
			car.RPCSingleParam(-3999349, null, true); // Unlock and reset
		}

		if (action_data.m_MainItem)
		{
			action_data.m_MainItem.SetHealth(0);
		}
	}

	override void OnEndServer( ActionData action_data )
	{
		if (action_data.m_Callback && action_data.m_Callback.GetState() != ACTION_STATE_FINISHED)
		{
			CarScript car = CarScript.Cast(action_data.m_Target.GetParent());
			if (car)
			{
				car.RPCSingleParam(-3999348, new Param1<bool>(true), true); // Keep/Restart Alarm
			}

			if (action_data.m_MainItem)
			{
				action_data.m_MainItem.SetHealth(0);
			}
		}
	}

	override bool ActionConditionContinue( ActionData action_data )
	{
		return true;
	}
}