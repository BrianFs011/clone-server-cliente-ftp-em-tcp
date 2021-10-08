#include <iostream>
#include <WS2tcpip.h>
#include <direct.h>
#include <map>
#include <fstream> 
#include <sstream>
#include <dirent.h>
#include <vector>
#include <thread>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

struct Dec {
	string version;
	string command;
	int size;
	char status;
	string secondCommand;
};

void funVersion();
SOCKET funListening();
sockaddr_in funHint();
int funBind(SOCKET, sockaddr_in);
int funListen(SOCKET);
string creatFolder(string, string);
int shutdown(SOCKET);
Dec decodeServerMessage(string);
string convertCommandForCliente(string, string, string);


bool dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

class thParams {
public:
	int clientsocket;
	string checkFolder;
	string path;
	HANDLE hThread;
	string cliente;
	SOCKET listening;

	void paramters(string checkFolder, int clientsocket, string path, HANDLE hThread, string cliente, SOCKET listening);
};

void thParams::paramters(string checkFolder, int clientsocket, string path, HANDLE hThread, string cliente, SOCKET listening) {
	this->checkFolder = checkFolder;
	this->clientsocket = clientsocket;
	this->path = path;
	this->hThread = hThread;
	this->cliente = cliente;
	this->listening = listening;

}

HANDLE hThread;
DWORD WINAPI threads(LPVOID );

int main()
{
    SOCKET clientSocket = INVALID_SOCKET;

    cout << "BRIAN'S SERVER" << endl;

	//versão socket
	funVersion();
	//listening
	SOCKET listening = funListening();
	//hint
	sockaddr_in hint = funHint();
	//bind
	funBind(listening, hint);

	cout << "Esperando conexão " << endl;

	while (true) {
		// listening
		int lis = funListen(listening);
		
		char service[NI_MAXHOST];//cliente conectado no servidor
		char host[NI_MAXHOST]; //nome do cliente
		//esperar a cone��o
		sockaddr_in client;
		int clientSize = sizeof(client);
		clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

		ZeroMemory(host, NI_MAXHOST);
		ZeroMemory(service, NI_MAXHOST);

		if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
			cout << host << " conectado na porta " << service << endl;
		}
		else {
			inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
			cout << host << " conectado na porta " << ntohs(client.sin_port) << endl;
		}
		//convert os dados do cliente 
		string cliente = host;
		cliente += " - ";
		cliente += (service);

		//cria a pasta do servidor
		string path = "c:\\BrianFTPserver\\";
		string checkFolder;
		checkFolder = creatFolder(path, checkFolder);

		thParams* par = new thParams();
		//manda os dados do cliente para a thread
		par->paramters(checkFolder, clientSocket, path, hThread, cliente, listening);

		//cria a thead
		CreateThread(NULL, 0, threads, (LPVOID)par, 0, NULL);

	}

	//desconect o servidor 
	shutdown(clientSocket);

	closesocket(listening);
	closesocket(clientSocket);

	WSACleanup();

	return 0;
}

void funVersion() {
	WSADATA wsData;
	int wsOk = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (wsOk != 0) {
		cerr << "N�o pode inicializar o winsock! Fechando" << endl;
	}
}

SOCKET funListening() {
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		cerr << "N�o pode inicializar o socket! Fechando" << endl;
		return 1;
	}
	return listening;
}

sockaddr_in funHint() {
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(41000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;
	return hint;
}

int funBind(SOCKET listening, sockaddr_in hint) {
	int erro = bind (listening, (sockaddr*)&hint, sizeof(hint));
	if (erro == SOCKET_ERROR) {
		printf("Bind falhou. Erro: %d\n", WSAGetLastError());
		closesocket(listening);
		WSACleanup();
	}
	return erro;
}

int funListen(SOCKET listening) {
	int lis = listen(listening, SOMAXCONN);
	if (lis == SOCKET_ERROR) {
		printf("Listen falhou. Erro: %d\n", WSAGetLastError());
		closesocket(listening);
		WSACleanup();
		return 1;
	}
	return lis;
}

string creatFolder(string path, string checkFolder) {
	if (_mkdir(path.c_str()) == -1) {
		checkFolder = "Conectado - path: " + path;

	}
	else {
		_mkdir(path.c_str());
		checkFolder = "Conectado - um novo diretorio foi criado - path: " + path;

	}
	return checkFolder;
}

int shutdown(SOCKET clientSocket) {
	int shut = shutdown(clientSocket, SD_SEND);
	if (shut == SOCKET_ERROR) {
		printf("Shutdown falhou. Erro: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}
}

DWORD WINAPI threads(LPVOID parameter) {
	thParams* parms = (thParams*)parameter;

	HANDLE closeHthread = parms->hThread;

	string clienteId = parms->cliente;
	string varPath = parms->path;
	map<int, string>paths;
	int cont = 1;
	paths.insert(pair<int, string>(0, "c:"));
	paths.insert(pair<int, string>(1, "BrianFTPserver"));


	//envia uma confirmação para o cliente de que o servidor está conectado 
	send(parms->clientsocket, parms->checkFolder.c_str(), parms->checkFolder.size(), 0);

	string condition;
	int size = 3012;
	char* buff = new char[size];
	while (condition != "exit") {

		//recebe dados do cliente
		ZeroMemory(buff, size);
		recv(parms->clientsocket, buff, size, 0);

		//decodifica dados do cliente
		
		string command = decodeServerMessage(buff).command;
		
		string order = decodeServerMessage(buff).secondCommand;
		
		string msg;
		if (command == "mkdir ") {
			string mkcomand = varPath + "\\" + order;

			if (_mkdir(mkcomand.c_str()) == -1) {
				

				msg = "pasta ja existe";


			}
			else {
				_mkdir(mkcomand.c_str());
				msg = "pasta " + order + " criada";
			}
		}

		else if (command == "get   ") {
			
			string getPath = varPath + order;

			ifstream arqEnvio;

			arqEnvio.open(getPath.c_str(), ifstream::binary);

			if (arqEnvio.fail()) {

				msg = "erro - arquivo não encontrado";
			}
			else {

				msg = "arquivo encontrado";

				//1 - vai dizer ao cliente que o arquivo esta pronto para envio

				cout << msg << endl;
				arqEnvio.seekg(0, arqEnvio.end);
				int tamanho = arqEnvio.tellg();
				int fragment = tamanho / size;
				int resFragment = tamanho - (fragment * size);

				string msgTamanho;
				stringstream ss;
				ss << tamanho;
				ss >> msgTamanho;

				msg = convertCommandForCliente(command, msgTamanho, "i");

				//2 - manda o tamho do arquivo
				send(parms->clientsocket, msg.c_str(), msg.size(), 0);

				arqEnvio.seekg(0);

				string barra;
				int cond = fragment / 10;
				int cond1 = fragment / 100;
				int c = 0;
				for (int i = 0; i <= fragment; i++) {
					system("CLS");

					cout << "Transferindo para " << parms->cliente << endl;

					cout << "[ ";
					if (i >= cond) {
						barra += "-";
						cond += fragment / 10;
						c += 10;


					}
					if (i == fragment) {
						c = 100;
					}

					cout << barra << " ]" << " " << c << " %" << endl;
					arqEnvio.read(buff, size);
					send(parms->clientsocket, buff, size, 0);

				}


				if (resFragment != 0) {
					arqEnvio.read(buff, resFragment);
					send(parms->clientsocket, buff, resFragment, 0);

				}

				cout << clienteId << " requisitou um arquivo: " << order << endl;
				msg = "arquivo enviado com sucesso.";

				arqEnvio.close();

			}

		}

		else if (command == "put   ") {

			ZeroMemory(buff, size);
			recv(parms->clientsocket, buff, size, 0);

			cout << buff << endl;

			if (decodeServerMessage(buff).secondCommand[0] == 'e' && decodeServerMessage(buff).secondCommand[1] == 'r' && decodeServerMessage(buff).secondCommand[2] == 'r' && decodeServerMessage(buff).secondCommand[3] == 'o') {
				msg = decodeServerMessage(buff).secondCommand;
			}
			else {


				string fileSize = decodeServerMessage(buff).secondCommand;
				int fragment = atoi (fileSize.c_str()) / 3000;

				int resFragment = atoi (fileSize.c_str()) - (fragment * 3000);

				ofstream arqRecebido;

				string caminhoDoArquivo = varPath + order;

				arqRecebido.open(caminhoDoArquivo.c_str(), ofstream::binary);

				string barra;
				int cond = fragment / 10;
				int cond1 = fragment / 100;
				int c = 0;
				string recs;
				
				for (int i = 0; i <= fragment; i++) {
					recv(parms->clientsocket, buff, size, 0);
					system("CLS");
					cout << "Copiando para " << caminhoDoArquivo << endl;

					cout << "[ ";
					if (i >= cond) {
						barra += "-";
						cond += fragment / 10;
						c += 10;

					}

					if (i == fragment) {
						c = 100;
					}

					cout << barra << " ]" << " " << c << " %" << endl;

					
					arqRecebido.write(buff, 3000);
			
					
				}
				if (resFragment != 0) {
					recv(parms->clientsocket, buff, resFragment, 0);
					arqRecebido.write(buff, resFragment);

				}

				arqRecebido.close();

				cout << clienteId << " enviou um arquivo: " << caminhoDoArquivo << endl;
				msg = "arquivo recebido com sucesso";
			}

		}

		else if (command == "ls    ") {
			if (dirExists(varPath)) {
				DIR* dir; struct dirent* diread;
				vector<string> files;
				if ((dir = opendir(varPath.c_str())) != nullptr) {

					while ((diread = readdir(dir)) != nullptr) {

						string tmp_string(diread->d_name);
						files.push_back(tmp_string);

					}
					closedir(dir);
				}
				else {
					perror("opendir");
					return EXIT_FAILURE;
				}

				for (int i = 0; i < files.size(); i++) {

					if (i == 0) {
						msg = "\n";
					}
					msg += files[i] + "\n";

				}
			}
			else {
				msg = "diretorio pode ter sido deletado";
			}

		}

		else if (command == "cd    ") {
			if (order == "..") {

				if (cont == 1) {
					msg = "fim da linha";
				}
				else {
					cont--;
					varPath = "";
					for (int i = 0; i <= cont; i++) {
						if (i == 0) {
							varPath += paths[i] + "\\";
						}
						else {
							varPath += paths[i] + "\\";
						}
					}
					msg = varPath;
					cout << clienteId << " entrou em " << varPath << endl;
				}
			}
			else if (order == "") {
				cont = 1;
				varPath = paths[0] + "\\" + paths[1] + "\\";

				paths.clear();
				paths.insert(pair<int, string>(0, "c:"));
				paths.insert(pair<int, string>(cont, "BrianFTPserver"));
				msg = varPath;
				cout << clienteId << " entrou em " << varPath << endl;

			}
			else {
				string concatenation = varPath + order;
				if (dirExists(concatenation)) {
					cont++;
					paths.insert(pair<int, string>(cont, order));
					varPath = "";
					for (int i = 0; i <= cont; i++) {
						if (i == 0) {
							varPath += paths[i] + "\\";
						}
						else {
							varPath += paths[i] + "\\";
						}

					}
					msg = varPath;
					cout << clienteId << " entrou em " << varPath << endl;
				}
				else {
					msg = "diretorio nao existe";
				}
				
			}
		}

		else if (command == "rmdir ") {
			string mkcomand = varPath + order;
			if (_rmdir(mkcomand.c_str()) == -1) {
				msg = "diretorio não existe ou não esta vazio";
			}
			else {
				_rmdir(mkcomand.c_str());
				msg = "diretorio removido com sucesso";
			}
		}

		else if (command == "delete") {
			string mkcomand = varPath + order;
			remove(mkcomand.c_str());
			msg = "arquivo removido com sucesso";
			cout << msg << " " << mkcomand << endl;
		}

		else if (command == "cls   ") {
			msg = "Conectado " + varPath;
		}

		else if (command == "close ") {
			msg = convertCommandForCliente(command, "desconectado!", "f");

			send(parms->clientsocket, msg.c_str(), msg.size(), 0);
			cout << parms->cliente << " desconectou com sucesso." << endl;
			shutdown(parms->clientsocket, SD_SEND);
			closesocket(parms->clientsocket);
			condition = "exit";
		}

		else if (command == "help  ") {
			msg = "comandos disponiveis\ncd\nls\nmkdir\nrmdir\ndelete\ncls\nclose\nquit ";
		}

		else {
			msg = "opcao nao existe - digite help";
		}

		msg = convertCommandForCliente(command, msg, "f");

		send(parms->clientsocket, msg.c_str(), msg.size(), 0);
	}
	delete[] buff;
	cout << "Thread encerrada" << endl;
	return 0;
}

Dec decodeServerMessage(string message) {
	Dec dec;

	dec.version = message[0];
	dec.command = message.substr(1, 6);
	dec.size = atoi(message.substr(7, 4).c_str());
	dec.status = message[11];
	if (message.length() > 12) {
		dec.secondCommand = message.substr(12, dec.size);
	}
	return dec;
}

string convertCommandForCliente(string command, string secondCommand, string status) {
	string walkThroughProgram = "1";
	int commandSize = 6 - command.length();
	walkThroughProgram += command;
	for (int i = 0; i < commandSize; i++) {
		walkThroughProgram += " ";
	}
	int secondCommandSize = secondCommand.length();
	string convertSizeSc = to_string(secondCommandSize);
	walkThroughProgram += convertSizeSc;
	for (int i = 0; i < 4 - convertSizeSc.length(); i++) {
		walkThroughProgram += " ";
	}
	walkThroughProgram += status;
	walkThroughProgram += secondCommand;

	return walkThroughProgram;

}