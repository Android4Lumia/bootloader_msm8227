/* tools/mkbootimg/bootimg.h
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _LIB_CMDLINE_H_
#define _LIB_CMDLINE_H_

#define CMDLINE_ADD(str) ptr+=strlcpy((ptr), (str), cmdline_size)
#define CMDLINE_BASEBAND(type, str) \
	case (type): \
		cmdline_add("androidboot.baseband", (str)); \
		break;

void cmdline_add(const char* name, const char* value, bool overwrite);
size_t cmdline_length(void);
size_t cmdline_generate(char* buf, size_t bufsize);
void cmdline_addall(char* cmdline, bool overwrite);
void cmdline_init(void);


#endif
