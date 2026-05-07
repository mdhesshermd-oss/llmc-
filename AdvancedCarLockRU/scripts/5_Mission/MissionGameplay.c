modded class MissionGameplay
{
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		PlayerBase playerPB = PlayerBase.Cast(GetGame().GetPlayer());
		Input input = GetGame().GetInput();

		if (input.LocalPress("CARCODELOCKTOGGLE", false))
		{
			if (playerPB)
				playerPB.TryOpenCarRemote();
		}
	}
}