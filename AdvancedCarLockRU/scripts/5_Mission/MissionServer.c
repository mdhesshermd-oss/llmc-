modded class MissionServer
{
	private const string ADMIN_FILE_PATH = "$profile:AdvancedCarLock/Admins.txt";
	private ref array<string> m_AdminList;

	override void OnInit()
	{
		super.OnInit();
		LoadAdmins();
	}

	void LoadAdmins()
	{
		m_AdminList = new array<string>;
		if (!FileExist(ADMIN_FILE_PATH))
		{
			if (!FileExist("$profile:AdvancedCarLock/"))
				MakeDirectory("$profile:AdvancedCarLock/");

			FileHandle file = OpenFile(ADMIN_FILE_PATH, FileMode.WRITE);
			if (file != 0)
			{
				FPrintln(file, "// Добавьте SteamID64 администраторов по одному на строку");
				FPrintln(file, "76561197960287930"); // Пример
				CloseFile(file);
			}
		}

		FileHandle file_read = OpenFile(ADMIN_FILE_PATH, FileMode.READ);
		if (file_read != 0)
		{
			string line;
			while (FGets(file_read, line) > 0)
			{
				line.Trim();
				if (line == "" || line.Contains("//")) continue;
				m_AdminList.Insert(line);
			}
			CloseFile(file_read);
		}
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);

		if (player && identity)
		{
			string steamId = identity.GetPlainId();
			if (m_AdminList && m_AdminList.Find(steamId) != -1)
			{
				player.SetImmobilizerAdmin(true);
			}
			else
			{
				player.SetImmobilizerAdmin(false);
			}
		}
	}
}
