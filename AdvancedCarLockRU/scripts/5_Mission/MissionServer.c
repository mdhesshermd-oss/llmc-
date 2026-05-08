modded class MissionServer
{
    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        AdvancedCarLockManager.GetInstance().SyncToPlayer(player);
    }
}
