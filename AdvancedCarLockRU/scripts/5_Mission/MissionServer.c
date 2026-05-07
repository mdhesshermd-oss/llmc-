modded class MissionServer
{
	private const string ADMIN_FILE_PATH = "$profile:AdvancedCarLock/Admins.txt";
	private const string LOG_FILE_PATH = "$profile:AdvancedCarLock/Logs.txt";
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

	static void LogLockpick(string playerName, string carType, bool success)
	{
		string status = "FAILED";
		if (success) status = "SUCCESS";

		int year, month, day, hour, minute, second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		string time = string.Format("%1/%2/%3 %4:%5:%6", day.ToStringLen(2), month.ToStringLen(2), year, hour.ToStringLen(2), minute.ToStringLen(2), second.ToStringLen(2));
		string logEntry = string.Format("[%1] Player '%2' attempted to lockpick '%3' - Result: %4", time, playerName, carType, status);

		FileHandle file = OpenFile("$profile:AdvancedCarLock/Logs.txt", FileMode.APPEND);
		if (file != 0)
		{
			FPrintln(file, logEntry);
			CloseFile(file);
		}
	}
}
