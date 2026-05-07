enum CarLockAction
{
	CREATECODE,
	ENTERCODE
}

class CarLockUI : UIScriptedMenu
{

	private EditBoxWidget editbox;
	private ButtonWidget enter;
	private ButtonWidget clear;

	private int actionType;
	private CarScript currentCar;

	private const string NUM_SAMPLE = "num_";
	private int lastCode;
	private bool confirmPassword;

	private PlayerBase player;

	void CarLockUI(int type, CarScript car)
	{
		actionType = type;
		currentCar = car;
		player = PlayerBase.Cast(GetGame().GetPlayer());
		GetGame().GetMission().PlayerControlDisable( INPUT_EXCLUDE_ALL );
	}

	void ~CarLockUI()
	{
		GetGame().GetMission().PlayerControlEnable( true );
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		Input input = GetGame().GetInput();

		if ( input.LocalPress( "UAUIBack", false ) )
			Close();
	}

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "AdvancedCarLockRU/assets/layouts/codelock.layout" );
		CastTo(editbox, layoutRoot.FindAnyWidget("editbox"));
		CastTo(enter, layoutRoot.FindAnyWidget("btnEnter"));
		CastTo(clear, layoutRoot.FindAnyWidget("btnClear"));

		return layoutRoot;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!ButtonWidget.Cast(w))
			return false;
		if (w == enter)
		{
			TryEnter();
			return true;
		}
		if (w == clear)
		{
			ClearDisplay();
			return true;
		}

		string num = w.GetName();
		if (num.Contains(NUM_SAMPLE))
		{
			num.Replace("num_", "");
			AddSymbol(num);
		}
		return true;
	}

	void AddSymbol(string char)
	{
		if (editbox.GetText().Contains("REPEAT"))
			editbox.SetText("");

		string current = editbox.GetText();
		editbox.SetText(current + char);
	}

	void OnPinVerified(bool success)
	{
		if (success)
		{
			player.SetCurrentCar(currentCar);
			currentCar.LogIn();
			GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Доступ разрешен", ""));
		}
		else
		{
			GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Доступ запрещен", ""));
		}
		Close();
	}

	void TryEnter()
	{
		string code = editbox.GetText();
		if (IsValidCode(code))
		{
			int intCode = code.ToInt();
			if (actionType == CarLockAction.CREATECODE)
			{
				if (!confirmPassword)
				{
					lastCode = intCode;
					confirmPassword = true;
					editbox.SetText("REPEAT");
				}
				else
				{
					if (lastCode == intCode)
					{
						currentCar.RPCSingleParam(-3999346, new Param1<int>(intCode), true);
						GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Новый пароль установлен: "+lastCode, ""));
						Close();
					}
					else
					{
						confirmPassword = false;
						editbox.SetText("ERROR");
						GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Пароли отличаются ", ""));
					}
				}
			}
			else
			{
				currentCar.RPCSingleParam(-3999353, new Param1<int>(intCode), true);
			}
		}
		else
		{
			confirmPassword = false;
			GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", "Код должен состоять из 4 - 8 цифр", ""));
			ClearDisplay();
		}
	}

	bool IsValidCode(string code)
	{
		int num;
		if (code.Length() < 4 || code.Length() > 8)
			return false;
		num = code.ToInt();
		if (num < 1000)
			return false;
		return true;
	}

	void ClearDisplay()
	{
		editbox.SetText(string.Empty);
	}
}