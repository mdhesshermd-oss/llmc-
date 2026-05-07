modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(ActionInteractWithLockOnCarDoor);
		actions.Insert(ActionDestroyCarLock);
		actions.Insert(ActionDirectOpenCarLock);
		actions.Insert(ActionDirectLockCarLock);

		actions.Insert(ActionAdminShowCarLockCode);
		actions.Insert(ActionAdminResetCarLockCode);
	}
}