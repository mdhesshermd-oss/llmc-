modded class ActionOpenCarDoorsOutside
{
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if (super.ActionCondition(player, target, item))
		{
			if (GetGame().IsServer())
				return true;

			CarScript car;
			if (!CastTo(car, target.GetParent()))
				CastTo(car, player.GetCommand_Vehicle().GetTransport());

			if (car)
			{
				return !car.IsLocked();
			}
		}
		return false;
	}
}