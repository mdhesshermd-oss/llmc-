class AdvancedCarLockConfig
{
    string carUnlockToolType = "Lockpick";
    float carUnlockDuration = 60.0;

    void AdvancedCarLockConfig() {}
}

class AdvancedCarLockManager
{
    private static ref AdvancedCarLockManager m_Instance;
    private ref AdvancedCarLockConfig m_Config;
    private ref array<string> m_Admins;
    private const static string m_ConfigPath = "$profile:AdvancedCarLock/Config.json";
    private const static string m_AdminsPath = "$profile:AdvancedCarLock/Admins.txt";

    void AdvancedCarLockManager()
    {
        m_Config = new AdvancedCarLockConfig();
        m_Admins = new array<string>;

        if (GetGame().IsServer())
        {
            LoadConfig();
            LoadAdmins();
        }
    }

    static AdvancedCarLockManager GetInstance()
    {
        if (!m_Instance)
            m_Instance = new AdvancedCarLockManager();
        return m_Instance;
    }

    void LoadConfig()
    {
        if (FileExist(m_ConfigPath))
        {
            JsonFileLoader<AdvancedCarLockConfig>.JsonLoadFile(m_ConfigPath, m_Config);
        }
        else
        {
            if (!FileExist("$profile:AdvancedCarLock/"))
                MakeDirectory("$profile:AdvancedCarLock/");
            JsonFileLoader<AdvancedCarLockConfig>.JsonSaveFile(m_ConfigPath, m_Config);
        }
    }

    void LoadAdmins()
    {
        if (FileExist(m_AdminsPath))
        {
            FileHandle file = OpenFile(m_AdminsPath, FileMode.READ);
            if (file)
            {
                string line;
                while (FGets(file, line) > 0)
                {
                    line.Trim();
                    if (line != "")
                        m_Admins.Insert(line);
                }
                CloseFile(file);
            }
        }
        else
        {
            if (!FileExist("$profile:AdvancedCarLock/"))
                MakeDirectory("$profile:AdvancedCarLock/");
            FileHandle file = OpenFile(m_AdminsPath, FileMode.WRITE);
            if (file)
            {
                FPrintln(file, "76561197960287930"); // Пример SteamID64
                CloseFile(file);
            }
        }
    }

    bool IsAdmin(string steamID)
    {
        return m_Admins.Find(steamID) != -1;
    }

    string GetUnlockTool()
    {
        return m_Config.carUnlockToolType;
    }

    float GetUnlockDuration()
    {
        return m_Config.carUnlockDuration;
    }

    void SyncToPlayer(PlayerBase player)
    {
        if (!player || !player.GetIdentity()) return;

        bool isAdmin = IsAdmin(player.GetIdentity().GetPlainId());
        auto params = new Param3<bool, string, float>(isAdmin, GetUnlockTool(), GetUnlockDuration());
        GetGame().RPCSingleParam(player, -999997865, params, true, player.GetIdentity());
    }
}
