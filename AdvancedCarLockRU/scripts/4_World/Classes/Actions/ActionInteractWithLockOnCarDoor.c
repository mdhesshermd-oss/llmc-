class ActionInteractWithLockOnCarDoor : ActionCarDoorsOutside
{
	private const string CREATE_CODE = "Создать код";
	private const string ENTER_CODE = "Ввести код";
	private string actionText;

	private ref CarLockUI lockui;

	override string GetText()
	{
		return actionText;
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
		if (!car.HasCode() && player.GetCurrentCar() == car)
		{
			actionText = CREATE_CODE;
			return true;
		}
		else if (car.HasCode() && !car.IsLogged())
		{
			actionText = ENTER_CODE;
			return true;
		}
		return false;
	}

	override bool CanBeUsedInVehicle()
	{
		return false;
	}

	override void OnStartClient( ActionData action_data )
	{
		TryOpenUI(action_data);
	}

	void TryOpenUI(ActionData action_data)
	{
		CarScript car;
		int type;
		car = CarScript.Cast(action_data.m_Target.GetParent());

		if (GetGame().GetUIManager().GetMenu() || lockui)
		return;
		if (!car)
			return;

		type = GetActionType(actionText);

		lockui = new CarLockUI(type, car);
		GetGame().GetUIManager().ShowScriptedMenu( lockui, NULL );
	}

	int GetActionType(string action)
	{
		int type;
		switch (action)
		{
			case CREATE_CODE:
				type = 0;
			break;
			case ENTER_CODE:
				type = 1;
			break;
			default:
				type = -1;
		}
		return type;
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