// Copyright 2020 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "lib.hpp"
#include "version.hpp"
#include "util-platform.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

#if defined(_WIN32)
#include "Windows.h"

#include "Shlobj.h"

#include "Knownfolders.h"
#endif

#define LOG_PREFIX "[VoiceFX]"

static bool _initialized = false;

static std::filesystem::path _user_data;
static std::filesystem::path _local_data;

static std::ofstream _log_stream;
static std::mutex    _log_stream_mutex;

std::string formatted_time(bool file_safe = false)
{
	// Get the current time.
	auto now = std::chrono::system_clock::now();

	// Preallocate a 32-byte buffer.
	std::vector<char> time_buffer(32, '\0');

	// Figure out if we want a file safe or log safe format.
	std::string_view local_format = "%04d-%02d-%02dT%02d:%02d:%02d.%06d";
	if (file_safe) {
		local_format = "%04d-%02d-%02dT%02d-%02d-%02d-%06d";
	}

	// Convert the current time into UTC.
	auto      nowt    = std::chrono::system_clock::to_time_t(now);
	struct tm tstruct = *gmtime(&nowt);
	auto      mis     = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());

	// Store the time according to the requested format.
	snprintf(time_buffer.data(), time_buffer.size(), local_format.data(), tstruct.tm_year + 1900, tstruct.tm_mon + 1, tstruct.tm_mday, tstruct.tm_hour, tstruct.tm_min, tstruct.tm_sec, mis.count() % 1000000);

	return std::string(time_buffer.data());
}

void voicefx::initialize()
{
	if (_initialized) {
		return;
	}

	// Pre-calculate and create storage directories.
	_local_data = util::platform::data_path() / voicefx::product_vendor / voicefx::product_name;
	_user_data  = util::platform::config_path() / voicefx::product_vendor / voicefx::product_name;
	std::filesystem::create_directories(_local_data);
	std::filesystem::create_directories(_user_data);

	// Try and open a log file
	{
		// Create the directory for log files if it happens to be missing.
		std::filesystem::path log_path = std::filesystem::path(_local_data) / "logs";
		std::filesystem::create_directories(log_path);

		// Create the log file itself.
		std::filesystem::path log_file = std::filesystem::path(log_path).append(formatted_time(true) + ".log");
		_log_stream                    = std::ofstream(log_file, std::ios::trunc | std::ios::out);

		// Clean up old files.
		try { // Delete all log files older than 1 month.
			for (auto& entry : std::filesystem::directory_iterator(log_path)) {
				try {
					auto wt = entry.last_write_time();
					if ((decltype(wt)::clock::now() - wt) > std::chrono::hours(24 * 14)) {
						std::filesystem::remove(entry);
					}
				} catch (std::exception const& ex) {
					voicefx::log("Failed to delete old log file '%s'.", entry.path().c_str());
				}
			}
		} catch (std::exception const& ex) {
			voicefx::log("Failed to clean up log file(s): %s", ex.what());
		}
	}

	voicefx::log("Loaded v" TONPLUGINS_VERSION ".");

#ifdef WIN32
	// Log information about the Host process.
	{
		std::vector<wchar_t> file_name_w(256, 0);
		size_t               file_name_len = 0;
		do {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				file_name_w.resize(file_name_w.size() * 2);
			}
			file_name_len = static_cast<DWORD>(GetModuleFileNameW(NULL, file_name_w.data(), static_cast<DWORD>(file_name_w.size())));
		} while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		std::string file_name = util::platform::native_to_utf8(std::wstring(file_name_w.data(), file_name_w.data() + file_name_len));
		voicefx::log("Host Process: %s (0x%08" PRIx32 ")", file_name.c_str(), GetCurrentProcessId());
	}
#endif

	_initialized = true;
}

std::filesystem::path voicefx::user_data()
{
	return _user_data;
}

std::filesystem::path voicefx::local_data()
{
	return _local_data;
}

void voicefx::log(const char* format, ...)
{
	// Build a UTF-8 string.
	std::string converted;

	{
		std::vector<char> format_buffer;
		std::vector<char> string_buffer;
		std::string       time = formatted_time();

		// Generate proper format string.
		{
			const char* local_format = "%s %s\n";

			size_t len = static_cast<size_t>(snprintf(nullptr, 0, local_format, time.data(), format)) + 1;
			format_buffer.resize(len);
			snprintf(format_buffer.data(), format_buffer.size(), local_format, time.data(), format);
		};

		{
			va_list args;
			va_start(args, format);
			size_t len = static_cast<size_t>(vsnprintf(nullptr, 0, format_buffer.data(), args)) + 1;
			string_buffer.resize(len);
			vsnprintf(string_buffer.data(), string_buffer.size(), format_buffer.data(), args);
			va_end(args);
		}

		converted = std::string{string_buffer.data(), string_buffer.size() - 1};
	}

	// Write to file and stdout.
	std::cout << LOG_PREFIX << converted;
	{
		std::lock_guard<std::mutex> lock(_log_stream_mutex);

		// This needs to be synchronous or bad things happen.
		if (_log_stream.good() && !_log_stream.bad()) {
			_log_stream << converted;
			_log_stream.flush();
		}
	}

#if defined(_WIN32) && defined(_MSC_VER)
	{ // Write to Debug console
		std::vector<char> string_buffer;

		{ // Need to prefix it as the debug console is noisy.
			const char* local_format = LOG_PREFIX " %s";

			size_t rfsz = static_cast<size_t>(snprintf(nullptr, 0, local_format, converted.data())) + 1;
			string_buffer.resize(rfsz);
			snprintf(string_buffer.data(), string_buffer.size(), local_format, converted.data());
		}

		// MSVC: Print to debug console
		std::vector<wchar_t> wstring_buffer(converted.size(), 0);
		size_t               len = static_cast<size_t>(MultiByteToWideChar(CP_UTF8, 0, string_buffer.data(), static_cast<int>(string_buffer.size()), wstring_buffer.data(), 0)) + 1;
		wstring_buffer.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, string_buffer.data(), static_cast<int>(string_buffer.size()), wstring_buffer.data(), static_cast<int>(wstring_buffer.size()));
		OutputDebugStringW(wstring_buffer.data());
#endif
	}
}
