#ifndef _PYTHON_HTTP_HPP_INCLUDED_
#define _PYTHON_HTTP_HPP_INCLUDED_

#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include "../json.hpp"

// Embedded Python HTTP client script.
// Reads a JSON request {url, method, data, headers} from --input file,
// performs the request via urllib, and writes a JSON result to --output file:
//   success  -> {"success":true,"status_code":int,"body":"...",
//                "headers":{name->val, "Set-Cookie" joined by \n},
//                "cookies":{name->val}}
//   failure  -> {"success":false,"status_code":int,"error":"...","body":"..."}
static const char* kHttpClientPy = R"PY(
import sys
import json
import urllib.request
import urllib.error

def main():
    input_file = None
    output_file = None

    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '--input' and i + 1 < len(sys.argv):
            input_file = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--output' and i + 1 < len(sys.argv):
            output_file = sys.argv[i + 1]
            i += 2
        else:
            i += 1

    if not input_file or not output_file:
        result = {"success": False, "error": "Usage: python http_client.py --input <input_file> --output <output_file>"}
        WriteResult(output_file, result)
        return

    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            request = json.load(f)
    except Exception as e:
        result = {"success": False, "error": f"Failed to read input file: {str(e)}"}
        WriteResult(output_file, result)
        return

    url = request.get("url", "")
    method = request.get("method", "GET")
    data = request.get("data", "")
    headers = request.get("headers", {})

    try:
        req = urllib.request.Request(
            url,
            data=data.encode('utf-8') if data else None,
            headers=headers,
            method=method
        )
        with urllib.request.urlopen(req, timeout=30) as response:
            status_code = response.getcode()
            body = response.read().decode('utf-8')

            resp_headers = {}
            for k, v in response.headers.items():
                resp_headers[k] = v
            set_cookies = response.headers.get_all('Set-Cookie') or []
            if set_cookies:
                resp_headers['Set-Cookie'] = "\n".join(set_cookies)

            cookies = {}
            for sc in set_cookies:
                first = sc.split(';', 1)[0].strip()
                if '=' in first:
                    name, val = first.split('=', 1)
                    cookies[name.strip()] = val.strip()

            result = {
                "success": True,
                "status_code": status_code,
                "body": body,
                "headers": resp_headers,
                "cookies": cookies
            }
            WriteResult(output_file, result)
    except urllib.error.HTTPError as e:
        try:
            body = e.read().decode('utf-8') if e.fp else ""
        except:
            body = ""
        result = {
            "success": False,
            "status_code": e.code,
            "error": str(e),
            "body": body
        }
        WriteResult(output_file, result)
    except Exception as e:
        result = {
            "success": False,
            "status_code": 0,
            "error": str(e)
        }
        WriteResult(output_file, result)

def WriteResult(output_file, result):
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(result, f, ensure_ascii=False)
    except Exception as e:
        print(json.dumps({"success": False, "error": f"Failed to write output: {str(e)}"}, ensure_ascii=False))

if __name__ == "__main__":
    main()
)PY";

class PythonHttpClient
{
public:
    PythonHttpClient(const std::string& pythonExe = "python")
        : m_pythonExe(pythonExe)
    {
        char tempFile[MAX_PATH];
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        GetTempFileNameA(tempPath, "py_", 0, tempFile);
        m_scriptPath = tempFile;

        std::ofstream ofs(m_scriptPath, std::ios::binary);
        if (ofs.is_open())
        {
            ofs.write(kHttpClientPy, static_cast<std::streamsize>(std::strlen(kHttpClientPy)));
            ofs.close();
        }
        else
        {
            throw std::runtime_error("Failed to create temporary Python script file");
        }
    }

    ~PythonHttpClient()
    {
        if (!m_scriptPath.empty())
        {
            DeleteFileA(m_scriptPath.c_str());
        }
    }

    bool SendRequest(
        const std::string& url,
        const std::string& method,
        const std::string& data,
        const std::map<std::string, std::string>& headers,
        int& statusCode,
        std::string& responseBody,
        std::string& errorMsg,
        std::map<std::string, std::string>* outHeaders = nullptr,
        std::map<std::string, std::string>* outCookies = nullptr)
    {
        std::string requestJson = BuildRequestJson(url, method, data, headers);

        char inputFile[MAX_PATH];
        char outputFile[MAX_PATH];
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        GetTempFileNameA(tempPath, "hin", 0, inputFile);
        GetTempFileNameA(tempPath, "hout", 0, outputFile);

        {
            std::ofstream ofs(inputFile);
            if (!ofs.is_open())
            {
                errorMsg = "Failed to create input file";
                DeleteFileA(inputFile);
                DeleteFileA(outputFile);
                return false;
            }
            ofs << requestJson;
            ofs.close();
        }

        std::string cmd = "\"" + m_pythonExe + "\" \"" + m_scriptPath + "\" --input \"" + inputFile + "\" --output \"" + outputFile + "\"";

        std::string errors;
        bool success = ExecuteCommand(cmd, errors);

        if (!success)
        {
            errorMsg = "Failed to execute Python script: " + errors;
            DeleteFileA(inputFile);
            DeleteFileA(outputFile);
            return false;
        }

        std::string output;
        {
            std::ifstream ifs(outputFile);
            if (!ifs.is_open())
            {
                errorMsg = "Failed to read output file";
                DeleteFileA(inputFile);
                DeleteFileA(outputFile);
                return false;
            }
            std::ostringstream ss;
            ss << ifs.rdbuf();
            output = ss.str();
            ifs.close();
        }

        DeleteFileA(inputFile);
        DeleteFileA(outputFile);

        if (ParseJsonResponse(output, statusCode, responseBody, errorMsg, outHeaders, outCookies))
        {
            return true;
        }
        else
        {
            errorMsg = "Failed to parse Python output: " + output;
            return false;
        }
    }

    bool Get(const std::string& url,
        const std::map<std::string, std::string>& headers,
        int& statusCode,
        std::string& responseBody,
        std::string& errorMsg,
        std::map<std::string, std::string>* outHeaders = nullptr,
        std::map<std::string, std::string>* outCookies = nullptr)
    {
        return SendRequest(url, "GET", "", headers, statusCode, responseBody, errorMsg, outHeaders, outCookies);
    }

    bool Post(const std::string& url,
        const std::string& data,
        const std::map<std::string, std::string>& headers,
        int& statusCode,
        std::string& responseBody,
        std::string& errorMsg,
        std::map<std::string, std::string>* outHeaders = nullptr,
        std::map<std::string, std::string>* outCookies = nullptr)
    {
        return SendRequest(url, "POST", data, headers, statusCode, responseBody, errorMsg, outHeaders, outCookies);
    }

private:
    std::string m_scriptPath;
    std::string m_pythonExe;

    std::string BuildRequestJson(const std::string& url, const std::string& method,
        const std::string& data, const std::map<std::string, std::string>& headers)
    {
        std::string json = "{";
        json += "\"url\":\"" + EscapeString(url) + "\",";
        json += "\"method\":\"" + EscapeString(method) + "\",";
        json += "\"data\":\"" + EscapeString(data) + "\",";

        json += "\"headers\":{";
        bool first = true;
        for (const auto& kv : headers)
        {
            if (!first) json += ",";
            json += "\"" + EscapeString(kv.first) + "\":\"" + EscapeString(kv.second) + "\"";
            first = false;
        }
        json += "}}";

        return json;
    }

    std::string EscapeString(const std::string& s)
    {
        std::string result;
        for (char c : s)
        {
            switch (c)
            {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
            }
        }
        return result;
    }

    bool ExecuteCommand(const std::string& cmd, std::string& errors)
    {
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStd_ERR_Rd = NULL;
        HANDLE hChildStd_ERR_Wr = NULL;

        if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0))
            return false;

        if (!SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0))
        {
            CloseHandle(hChildStd_ERR_Rd);
            CloseHandle(hChildStd_ERR_Wr);
            return false;
        }

        STARTUPINFOA siStartInfo;
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.hStdError = hChildStd_ERR_Wr;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        siStartInfo.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION piProcInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        std::string cmdBuffer = cmd;

        if (!CreateProcessA(
            NULL,
            const_cast<LPSTR>(cmdBuffer.c_str()),
            NULL, NULL, TRUE,
            CREATE_NO_WINDOW,
            NULL, NULL,
            &siStartInfo,
            &piProcInfo))
        {
            CloseHandle(hChildStd_ERR_Rd);
            CloseHandle(hChildStd_ERR_Wr);
            return false;
        }

        CloseHandle(hChildStd_ERR_Wr);

        DWORD dwRead;
        CHAR chBuf[4096];
        while (ReadFile(hChildStd_ERR_Rd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL) && dwRead > 0)
        {
            chBuf[dwRead] = '\0';
            errors += chBuf;
        }

        WaitForSingleObject(piProcInfo.hProcess, 30000);

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(hChildStd_ERR_Rd);

        return true;
    }

    // Parse the unified JSON written by the Python script.
    // Fields: success, status_code, body, error, headers{}, cookies{}.
    // Returns true when the JSON could be parsed (mirrors legacy semantics:
    // success OR an error message was present).
    bool ParseJsonResponse(const std::string& jsonText, int& statusCode, std::string& body, std::string& errorMsg,
        std::map<std::string, std::string>* outHeaders, std::map<std::string, std::string>* outCookies)
    {
        statusCode = 0;
        body.clear();
        errorMsg.clear();
        if (outHeaders) outHeaders->clear();
        if (outCookies) outCookies->clear();

        if (jsonText.empty())
        {
            errorMsg = "empty response";
            return false;
        }

        try
        {
            nlohmann::json j = nlohmann::json::parse(jsonText);
            bool success = j.value("success", false);
            statusCode = j.value("status_code", 0);
            if (j.contains("body") && j["body"].is_string())
                body = j["body"].get<std::string>();
            if (j.contains("error") && j["error"].is_string())
                errorMsg = j["error"].get<std::string>();

            if (outHeaders && j.contains("headers") && j["headers"].is_object())
            {
                for (auto it = j["headers"].begin(); it != j["headers"].end(); ++it)
                {
                    if (it.value().is_string())
                        (*outHeaders)[it.key()] = it.value().get<std::string>();
                }
            }
            if (outCookies && j.contains("cookies") && j["cookies"].is_object())
            {
                for (auto it = j["cookies"].begin(); it != j["cookies"].end(); ++it)
                {
                    if (it.value().is_string())
                        (*outCookies)[it.key()] = it.value().get<std::string>();
                }
            }

            return success || !errorMsg.empty();
        }
        catch (const std::exception& e)
        {
            errorMsg = std::string("parse error: ") + e.what();
            return false;
        }
    }
};

#endif // _PYTHON_HTTP_HPP_INCLUDED_
