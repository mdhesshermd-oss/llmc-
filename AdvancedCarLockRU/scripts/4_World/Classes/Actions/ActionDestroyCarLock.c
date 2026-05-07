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
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_PICKLOCK;
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
			car.SetAlarmState(true);
		}
	}

	override void OnFinishProgressServer( ActionData action_data )
	{
		CarScript car = CarScript.Cast(action_data.m_Target.GetParent());
		PlayerBase player = action_data.m_Player;

		if (car && player)
		{
			float chance = player.GetLockpickSuccessChance();
			float roll = Math.RandomFloat01();

			bool success = (roll <= chance);

			if (success)
			{
				car.ResetLock();
				MissionServer.LogLockpick(player.GetIdentity().GetName(), car.GetType(), true);
			}
			else
			{
				car.SetAlarmState(true);
				MissionServer.LogLockpick(player.GetIdentity().GetName(), car.GetType(), false);
			}
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
			PlayerBase player = action_data.m_Player;
			if (car)
			{
				car.SetAlarmState(true);
				if (player)
					MissionServer.LogLockpick(player.GetIdentity().GetName(), car.GetType(), false);
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