class CfgPatches
{
	class AdvancedCarLockRU_Scripts
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Data"
		};
	};
};
class CfgSoundShaders
{
	class CARLOCK_SoundShader_Base
	{
		range = 100;
	};
	class CARLOCK_Open_SoundShader: CARLOCK_SoundShader_Base
	{
		samples[] =
		{

			{
				"AdvancedCarLockRU\assets\sounds\OpenCar",
				1
			}
		};
		volume = 1;
	};
	class CARLOCK_Close_SoundShader: CARLOCK_SoundShader_Base
	{
		samples[] =
		{

			{
				"AdvancedCarLockRU\assets\sounds\CloseCar",
				1
			}
		};
		volume = 1;
	};
	class CARLOCK_Signal_SoundShader: CARLOCK_SoundShader_Base
	{
		samples[] =
		{

			{
				"AdvancedCarLockRU\assets\sounds\Signal",
				1
			}
		};
		volume = 1;
	};
};
class CfgSoundSets
{
	class CARLOCK_SoundSet_Base
	{
		sound3DProcessingType = "Vehicle_Ext_3DProcessingType";
		distanceFilter = "softVehiclesDistanceFilter";
		volumeCurve = "inverseSquare2Curve";
		spatial = 1;
		doppler = 0;
		loop = 0;
	};
	class CARLOCK_Open_SoundSet: CARLOCK_SoundSet_Base
	{
		soundShaders[] =
		{
			"CARLOCK_Open_SoundShader"
		};
	};
	class CARLOCK_Close_SoundSet: CARLOCK_SoundSet_Base
	{
		soundShaders[] =
		{
			"CARLOCK_Close_SoundShader"
		};
	};
	class CARLOCK_Signal_SoundSet: CARLOCK_SoundSet_Base
	{
		loop = 1;
		soundShaders[] =
		{
			"CARLOCK_Signal_SoundShader"
		};
	};
};
class CfgMods
{
	class AdvancedCarLockRU
	{
		dir="AdvancedCarLockRU";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="AdvancedCarLockRU";
		credits="Exiled, Jules";
		author="Exiled, Jules";
		authorID="0";
		version="1.1";
		extra=0;
		inputs="AdvancedCarLockRU/assets/inputs/modded_Inputs.xml";
		type="mod";
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"AdvancedCarLockRU/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"AdvancedCarLockRU/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"AdvancedCarLockRU/scripts/5_Mission"
				};
			};
		};
	};
};
