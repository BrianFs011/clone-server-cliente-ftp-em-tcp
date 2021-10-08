#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <direct.h>
#include <sstream>
#include <fstream> 
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define PORT      41000

struct Dec {
	string version;
	string command;
	int size;
	char status;
	string secondCommand;
};

void funVersion();
SOCKET funSocket();
sockaddr_in funStructure(string);
int funConnect(SOCKET, sockaddr_in);
string funCreatFolder(string, string);
string convertCommandForServer(string, string, string);
Dec decodeServerMessage(string);

int main()
{

	int size = 3012;
	char* buff = new char[size];

	string quitCliente, 
		ipAddress, 
		command,
		secondCommand,
		walkThroughProgram,
		path, 
		checkFolder;

	while (quitCliente != "quit") {

		cout << "FTP> ";
		cin >> command;

		if (command != "quit") {
			
			while (command != "open" ) {
				cout << "comando não existe - tente outra vez ";
				cin >> command;
				ipAddress = "0";
			}

			cin >> ipAddress;

			//versão socket
			funVersion();
			//socket
			SOCKET cSocket = funSocket();
			//hint
			sockaddr_in hint = funStructure(ipAddress);
			//connect
			int connected = funConnect(cSocket, hint);
			//condição se o connect falha não entra no programa
			if (connected == 1) {
				command = "close";
			}
			else {
			
				path = "c:\\BrianFTPcliente\\";
				checkFolder = funCreatFolder(path, checkFolder);

				ostringstream str1;
				str1 << cSocket;

				path += str1.str();
				checkFolder = funCreatFolder(path, checkFolder);

				cout << checkFolder << endl;

				ZeroMemory(buff, size);
				recv(cSocket, buff, size, 0);
				cout << "SERVER> " << buff << endl;
			
			}

			//loop do programa
			while (command != "close") {

				
				
				cout << "Digite um comando: ";
				//lê o comando
				cin >> command;

				if (command == "close") {
					walkThroughProgram = convertCommandForServer(command, "", "f");
					
				}
				else if (command == "quit") {
					quitCliente = command;
					command = "close";
					walkThroughProgram = convertCommandForServer(command, "", "f");
					
				}
				else if (command == "ls") {
					walkThroughProgram = convertCommandForServer(command, "", "f");
					
				}
				else if (command == "cd") {
					getline(cin, secondCommand);
					if (secondCommand == "") {
						walkThroughProgram = convertCommandForServer(command, "", "f");
						
					}
					else {
						int sizeSc = secondCommand.length();
						secondCommand = secondCommand.substr(1, sizeSc);
						walkThroughProgram = convertCommandForServer(command, secondCommand, "f");
						
					}
				}
				else if (command == "cls") {
					walkThroughProgram = convertCommandForServer(command, "", "f");

					system("CLS");
				}
				else if (command == "help") {
					walkThroughProgram = convertCommandForServer(command, "", "f");
				}
				//se o comando for composto aqui ele será concatenado 
				else {
					//vai colocar a segunda parte do comando a variavel 
					
					cin >> secondCommand;
				
					//a variavel que será enviada ao servido recebe Versão, comando, Tamanho e a segunda parte do comando  
					walkThroughProgram = convertCommandForServer(command, secondCommand,"f");
					
				}
				
				//envia os dados para o servidor
				send(cSocket, walkThroughProgram.c_str(), walkThroughProgram.size() + 1, 0);

				if (command == "put") {
					string fileName = path + "\\";
					string fn;
					

					for (int i = 0; i < secondCommand.size(); i++) {
						fileName += secondCommand[i];
						fn += secondCommand[i];
					}
					

					ifstream arqEnvio;


					arqEnvio.open(fileName.c_str(), ifstream::binary);

					if (arqEnvio.fail()) {
						
						walkThroughProgram = convertCommandForServer(command, "erro - arquivo não encontrado", "f");
						send(cSocket, walkThroughProgram.c_str(), walkThroughProgram.size() + 1, 0);


					}
					else {

						arqEnvio.seekg(0, arqEnvio.end);
						int tamanho = arqEnvio.tellg();
						int fragment = tamanho / 3000;

						string msgTamanho;
						stringstream ss;
						ss << tamanho;
						ss >> msgTamanho;
						
						int resFragment = tamanho - (fragment * 3000);

						msgTamanho = convertCommandForServer(command, msgTamanho, "i");

						//manda as info do arquivo
						send(cSocket, msgTamanho.c_str(), msgTamanho.size(), 0);

						arqEnvio.seekg(0);

						string barra;
						int cond = fragment / 10;
						int cond1 = fragment / 100;
						int c = 0;
						string sendMessage;
						
						for (int i = 0; i <= fragment; i++) {
							system("CLS");
					
							cout << "Transferindo para servidor" << endl;

							cout << "[ ";
							if (i >= cond) {
								barra += "-";
								cond += fragment / 10;
								c += 10;
								

							}
							if (i == fragment) {
								c = 100;
							}
							
							arqEnvio.read(buff, 3000);
							sendMessage = convertCommandForServer(command, buff, "m");
							cout << barra << " ]" << " " << c << " %" << endl;
					
							send(cSocket, buff, size, 0);
				

						}
						
						if (resFragment != 0) {
							arqEnvio.read(buff, resFragment);
							send(cSocket, buff, resFragment, 0);
							
						}
						

						
						arqEnvio.close();
					}

				}

				//recebe os dados do servidor
				ZeroMemory(buff, size);
				recv(cSocket, buff, size, 0);

				if (command == "get") {
					
					if (decodeServerMessage(buff).secondCommand[0] != 'e' && decodeServerMessage(buff).secondCommand[1] != 'r' && decodeServerMessage(buff).secondCommand[2] != 'r' && decodeServerMessage(buff).secondCommand[3] != 'o') {
				

						int tamanho = atoi (decodeServerMessage(buff).secondCommand.c_str());
						int fragment = tamanho / size;
						int resFragment = tamanho - (fragment * size);

						ofstream arqRecebido;


						string nomedoarquivo = path + "\\" + secondCommand;

						arqRecebido.open(nomedoarquivo.c_str(), ofstream::binary);


						string barra;
						int cond = fragment / 10;
						int cond1 = fragment / 100;
						int c = 0;

						for (int i = 0; i <= fragment; i++) {
							recv(cSocket, buff, size, 0);
							system("CLS");
							cout << "Copiando para " << nomedoarquivo << endl;

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


							arqRecebido.write(buff, size);
						}

						if (resFragment != 0) {
							recv(cSocket, buff, resFragment, 0);
							arqRecebido.write(buff, resFragment);
						}
						arqRecebido.close();

						ZeroMemory(buff, size);
						recv(cSocket, buff, size, 0);

						

					}

				}
				//mostra os dados do servido na tela 
				cout << "SERVER> " << decodeServerMessage(buff).secondCommand << endl;
			}
			
			closesocket(cSocket);
			WSACleanup();

		}
		else {

			quitCliente = command;
			cout << "cliente encerrado";

		}

	}

		delete[] buff;

}

void funVersion(){
	//Iniciar winsock
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
	}
}

SOCKET funSocket() {
	SOCKET cSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (cSocket == INVALID_SOCKET) {
		cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
		return 1;
	}
	return cSocket;
}

sockaddr_in funStructure(string ipAddress) {
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);
	return hint;
}

int funConnect(SOCKET cSocket, sockaddr_in hint) {
	int connResult = connect(cSocket, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(cSocket);
		WSACleanup();
		return 1;
	}
}

string funCreatFolder(string path, string checkFolder) {
	if (_mkdir(path.c_str()) == -1) {
		checkFolder = "Cliente> Diretório para downloads -> " + path;

	}
	else {
		_mkdir(path.c_str());
		checkFolder = "Cliente> Diretório para downloads criado -> " + path;

	}
	return checkFolder;
}

string convertCommandForServer(string command, string secondCommand, string status) {
	string walkThroughProgram = "1";
	int commandSize = 6 - command.length();
	walkThroughProgram += command;
	for (int i = 0; i < commandSize; i++) {
		walkThroughProgram += " ";
	}
	int secondCommandSize = secondCommand.length();
	string convertSizeSc = to_string(secondCommandSize);
	walkThroughProgram += convertSizeSc;
	for (int i = 0; i < 4-convertSizeSc.length(); i++) {
		walkThroughProgram += " ";
	}
	walkThroughProgram += status;
	walkThroughProgram += secondCommand;

	return walkThroughProgram;
	
}

Dec decodeServerMessage(string message) {
	Dec dec;

	dec.version = message[0];
	dec.command = message.substr(1,6);
	dec.size = atol(message.substr(6, 4).c_str());
	dec.status = message[11];
	if(message.length() > 12) {
		dec.secondCommand = message.substr(12, dec.size);
	}

	return dec;
}

























//1mkdir xxxxfteste123---------------------------------------