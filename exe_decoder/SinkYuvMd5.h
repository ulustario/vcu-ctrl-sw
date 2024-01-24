// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_app/Sink.h"

std::unique_ptr<IFrameSink> createYuvMd5Calculator(std::string path);
