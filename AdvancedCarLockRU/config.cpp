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
