/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "ExternalAI/Interface/SSAILibrary.h"
#include "ExternalAI/Interface/SGAILibrary.h"

#include <map>
#include <vector>
#include <string>

enum LevelOfSupport;
class SharedLib;

class CInterface {
public:
	CInterface();
	
	// static properties
	int GetInfos(InfoItem infos[], unsigned int max);
	LevelOfSupport GetLevelOfSupportFor(
			const char* engineVersion, int engineAIInterfaceGeneratedVersion);

	// skirmish AI methods
	const SSAILibrary* LoadSkirmishAILibrary(const struct InfoItem infos[], unsigned int numInfos);
	int UnloadSkirmishAILibrary(const SSAISpecifier* const sAISpecifier);
	int UnloadAllSkirmishAILibraries();

	// group AI methods
	const SGAILibrary* LoadGroupAILibrary(const struct InfoItem infos[], unsigned int numInfos);
	int UnloadGroupAILibrary(const SGAISpecifier* const gAISpecifier);
	int UnloadAllGroupAILibraries();

private:
	// these functions actually load and unload the libraries
	SharedLib* Load(const SSAISpecifier* const sAISpecifier, SSAILibrary* ai);
	SharedLib* LoadSkirmishAILib(const std::string& libFilePath, SSAILibrary* ai);

	SharedLib* Load(const SGAISpecifier* const gAISpecifier, SGAILibrary* ai);
	SharedLib* LoadGroupAILib(const std::string& libFilePath, SGAILibrary* ai);

	static void reportInterfaceFunctionError(const std::string& libFileName, const std::string& functionName);
	static void reportError(const std::string& msg);
	std::string GenerateLibFilePath(const SSAISpecifier& sAISpecifier);
	std::string GenerateLibFilePath(const SGAISpecifier& gAISpecifier);
	#define MAX_INFOS 128
	static SSAISpecifier extractSpecifier(const SSAILibrary& skirmishAILib);
	static SGAISpecifier extractSpecifier(const SGAILibrary& groupAILib);

	bool FitsThisInterface(const std::string& requestedShortName, const std::string& requestedVersion);
private:
	std::vector<InfoItem> myInfos;

	std::vector<SSAISpecifier> mySkirmishAISpecifiers;
	typedef std::map<SSAISpecifier, std::map<std::string, InfoItem>, SSAISpecifier_Comparator> T_skirmishAIInfos;
	T_skirmishAIInfos mySkirmishAIInfos;
	typedef std::map<SSAISpecifier, SSAILibrary*, SSAISpecifier_Comparator> T_skirmishAIs;
	T_skirmishAIs myLoadedSkirmishAIs;
	typedef std::map<SSAISpecifier, SharedLib*, SSAISpecifier_Comparator> T_skirmishAILibs;
	T_skirmishAILibs myLoadedSkirmishAILibs;

	std::vector<SGAISpecifier> myGroupAISpecifiers;
	typedef std::map<SGAISpecifier, std::map<std::string, InfoItem>, SGAISpecifier_Comparator> T_groupAIInfos;
	T_groupAIInfos myGroupAIInfos;
	typedef std::map<SGAISpecifier, SGAILibrary*, SGAISpecifier_Comparator> T_groupAIs;
	T_groupAIs myLoadedGroupAIs;
	typedef std::map<SGAISpecifier, SharedLib*, SGAISpecifier_Comparator> T_groupAILibs;
	T_groupAILibs myLoadedGroupAILibs;
};

#endif	/* _INTERFACE_H */
