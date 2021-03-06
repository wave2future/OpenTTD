/* $Id: script_info.cpp 17248 2009-08-21 20:21:05Z rubidium $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_info.cpp Implementation of ScriptFileInfo. */

#include "../stdafx.h"

#include <squirrel.h>
#include "squirrel.hpp"
#include "squirrel_helper.hpp"

#include "script_info.hpp"
#include "script_scanner.hpp"

ScriptFileInfo::~ScriptFileInfo()
{
	free((void *)this->author);
	free((void *)this->name);
	free((void *)this->short_name);
	free((void *)this->description);
	free((void *)this->date);
	free((void *)this->instance_name);
	free((void *)this->url);
	free(this->main_script);
	free(this->SQ_instance);
}

bool ScriptFileInfo::CheckMethod(const char *name) const
{
	if (!this->engine->MethodExists(*this->SQ_instance, name)) {
		char error[1024];
		snprintf(error, sizeof(error), "your info.nut/library.nut doesn't have the method '%s'", name);
		this->engine->ThrowError(error);
		return false;
	}
	return true;
}

/* static */ SQInteger ScriptFileInfo::Constructor(HSQUIRRELVM vm, ScriptFileInfo *info)
{
	/* Set some basic info from the parent */
	info->SQ_instance = MallocT<SQObject>(1);
	Squirrel::GetInstance(vm, info->SQ_instance, 2);
	/* Make sure the instance stays alive over time */
	sq_addref(vm, info->SQ_instance);
	ScriptScanner *scanner = (ScriptScanner *)Squirrel::GetGlobalPointer(vm);
	info->engine = scanner->GetEngine();

	static const char * const required_functions[] = {
		"GetAuthor",
		"GetName",
		"GetShortName",
		"GetDescription",
		"GetVersion",
		"GetDate",
		"CreateInstance",
	};
	for (size_t i = 0; i < lengthof(required_functions); i++) {
		if (!info->CheckMethod(required_functions[i])) return SQ_ERROR;
	}

	info->main_script = strdup(scanner->GetMainScript());

	/* Cache the data the info file gives us. */
	if (!info->engine->CallStringMethodStrdup(*info->SQ_instance, "GetAuthor", &info->author)) return SQ_ERROR;
	if (!info->engine->CallStringMethodStrdup(*info->SQ_instance, "GetName", &info->name)) return SQ_ERROR;
	if (!info->engine->CallStringMethodStrdup(*info->SQ_instance, "GetShortName", &info->short_name)) return SQ_ERROR;
	if (!info->engine->CallStringMethodStrdup(*info->SQ_instance, "GetDescription", &info->description)) return SQ_ERROR;
	if (!info->engine->CallStringMethodStrdup(*info->SQ_instance, "GetDate", &info->date)) return SQ_ERROR;
	if (!info->engine->CallIntegerMethod(*info->SQ_instance, "GetVersion", &info->version)) return SQ_ERROR;
	if (!info->engine->CallStringMethodStrdup(*info->SQ_instance, "CreateInstance", &info->instance_name)) return SQ_ERROR;

	/* The GetURL function is optional. */
	if (info->engine->MethodExists(*info->SQ_instance, "GetURL")) {
		if (!info->engine->CallStringMethodStrdup(*info->SQ_instance, "GetURL", &info->url)) return SQ_ERROR;
	}

	return 0;
}
