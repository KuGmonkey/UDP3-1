#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include<winsock2.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

//����
#define send_IP "127.0.0.1"
#define send_Port 6000
#define recv_IP "127.0.0.1"
#define recv_Port 53432
SOCKET sockSend;//�������Ͷ�socket�����󶨴����Э��UDP�����ݱ��׽��֣�IPv4
SOCKADDR_IN addrSend;//���巢�Ͷ˵�ַ
SOCKADDR_IN addrRecv;//������ն˵�ַ
const int max_size = 1024;//һ�����ݰ������������
const int SEND_TIMES = 10;//����ش�����
const int WAIT_TIME = 1;//��ʱ���ȴ�ʱ�䣬1s
int seq = 0;//�������к�
char* file_buf;//�����ļ�������
int file_len = 0;//�����ļ�����


//��ȡ��ǰϵͳ���ں�ʱ��
char* get_time()
{
	time_t now = time(0); // �� now ת��Ϊ�ַ�����ʽ 
	char* dt = ctime(&now);
	return dt;
}

//�������ݰ�
struct packet
{
	char flag;//��־λ '0':SYN;'1':ACK;'2':FIN
	bool is_final_file;//������ݰ��Ƿ�Ϊ�ļ����һ�� true:�ǣ�false:��
	bool not_empty;//���ݰ��ǿձ�־ true:�ǿ�;false:��
	DWORD send_ip, recv_ip;
	u_short send_port, recv_port;
	int Seq;//���
	int ACK;//Ӧ�����
	int len;//����
	u_short checksum;//16λ��У���
	char data[max_size];//�������

	packet(); //���캯��
};

//���ݰ���ʼ��
packet::packet() {
	//���������ݰ�����Ϊȫ��
	memset(this, 0, sizeof(packet));
	send_ip = DWORD(send_IP);
	recv_ip = DWORD(recv_IP);
	send_port = send_Port;
	recv_port = recv_Port;
	Seq = 0;
	ACK = 0;
	len = 0;
	checksum = 0;
}

//����У���
void set_checksum(packet& p)
{
	//У�������
	memset(&p.checksum, 0, sizeof(p.checksum));
	//�������ݰ����ж��ٸ�16�ֽ�����ȡ��
	int count = (sizeof(packet) + 1) / 2;
	//ָ���һ��16λ
	u_short* bit16 = (u_short*)(&p);
	u_long sum = 0;
	for (int i = 0; i < count; i++)
	{
		//ʵ�ַ������
		sum += *(bit16++);
		if (sum & 0xffff0000)
		{
			sum -= 0xffff;
			sum += 1;
		}
	}
	p.checksum = ~sum;
	return;
}

//��֤У���
bool is_checksum(packet& p) {
	int count = (sizeof(packet) + 1) / 2;
	u_short* bit16 = (u_short*)(&p);
	u_long sum = 0;
	for (int i = 0; i < count; i++) {
		sum += *(bit16++);
		if (sum & 0xffff0000) {
			sum -= 0xffff;
			sum += 1;
		}
	}
	if ((sum == 0xffff))
		return true;
	else
		return false;
}

//�������ͺ����������sendto������������ţ��ǿռ��飬У���
bool my_sendto(packet& packet) {
	packet.Seq = seq++;
	packet.not_empty = true;
	set_checksum(packet);
	int state;
	state = sendto(sockSend, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrRecv, sizeof(sockaddr));
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

//�������պ����������recvfrom������У��ͺͿհ����
bool my_recvfrom(packet& packet) {
	int addr_len = sizeof(sockaddr_in);
	memset(packet.data, 0, sizeof(packet.data));
	int state;
	//���յ��İ�����packet��
	state = recvfrom(sockSend, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrRecv, &addr_len);
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

//���ֽ���
bool send_Conn()//���Ͷˣ������������
{
	packet send_syn, recv_ack;
	bool state1,state2;

	//���ͽ�������
	send_syn.flag = '0';//SYN
	state1 = my_sendto(send_syn);
	if (state1)
		cout << "[log]" << get_time() << ">>>" << "������������ɹ�" << endl;
	else
	{
		cout << "[log]" << get_time() << ">>>" << "������������ʧ��" << endl;
		return false;
	}

	while (1)
	{
		//���ն˻�Ӧ
		state2 = my_recvfrom(recv_ack);
		if (state2)
		{
			cout << "[log]" << get_time() << ">>>" << "�յ���Ӧ�����ӳɹ���" << endl;
			return true;
		}
		else
		{
			cout << "[log]" << get_time() << ">>>" << "δ�յ���Ӧ������ʧ�ܣ�" << endl;
			return false;
		}
	}
}

//���ֶ���
bool send_Clo()
{
	packet send_fin, recv_fin;
	bool state1, state2;

	//���Ͷ�������
	send_fin.flag = '2';
	state1 = my_sendto(send_fin);
	if (state1)
		cout << "[log]" << get_time() << ">>>" << "���Ͷ�������ɹ�!" << endl;
	else
	{
		cout << "[log]" << get_time() << ">>>" << "���Ͷ�������ʧ��" << endl;
		return false;
	}

	while (1)
	{
		//���ն˻�Ӧ
		state2 = my_recvfrom(recv_fin);
		if (state2)
		{
			cout << "[log]" << get_time() << ">>>" << "�յ���Ӧ���Ͽ��ɹ���" << endl;
			return true;
		}
		else
		{
			cout << "[log]" << get_time() << ">>>" << "δ�յ���Ӧ���Ͽ�ʧ�ܣ�" << endl;
			return false;
		}
	}
}

//���ļ�
bool read_file()
{
	cout << "[log]" << get_time() << ">>>" << "�������ļ�·��:";
	string path;cin >> path;
	ifstream file;
	//���ļ�
	file.open(path, ifstream::in | ios::binary);

	if (file)
	{
		cout << "[log]" << get_time() << ">>>" << "��ʼ��ȡ�ļ�" << endl;

		//��ȡ�ļ���С
		//��ָ��ָ���ļ�ĩβ��ƫ��0
		file.seekg(0, file.end);
		//��ȡָ�뵱ǰλ��
		file_len = file.tellg();
		//�ָ�ָ��Ϊ�ļ���ʼ
		file.seekg(0, file.beg);
		cout << "[log]" << get_time() << ">>>" << "�ļ���СΪ " << file_len << " Byte" << endl;    //�ֽ�Ϊ��λ

		//��ȡ�ļ�������file����
		file_buf = new char[file_len + 1];
		file_buf[file_len] = '\0';
		file.read(file_buf, file_len);
		cout << "[log]" << get_time() << ">>>" << "�ļ���ȡ�ɹ�!" << endl;

		//�ر��ļ�
		file.close();
		return true;
	}
	else
	{
		cout << "[log]" << get_time() << ">>>" << "��ȡ�ļ�ʧ��" << endl;
		return false;
	}
}

//�ش�����(��Ų�����)
bool re_sendto(packet& packet)
{
	packet.not_empty = true;
	set_checksum(packet);
	int state;
	state = sendto(sockSend, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrRecv, sizeof(sockaddr));
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

//ͣ�ȷ���
void stop_wait_send()
{
	//�����ļ�
	cout << "[log]" << get_time() << ">>>" << "���ڷ���......" << endl;
	//����
	int packet_num = file_len / max_size + (file_len % max_size != 0);
	for (int i = 0; i < packet_num; i++)
	{
		cout << "���ͷ���" << i;
		packet send_packet, recv_packet;
		if (i != packet_num - 1)
		{
			send_packet.is_final_file = false;
			send_packet.len = max_size;
			for (int j = 0; j < send_packet.len; j++)
				send_packet.data[j] = file_buf[i * max_size + j];

			//ͣ�ȷ���
			//�״η���
			my_sendto(send_packet);

			int count = 0;    //�ش�����
			clock_t start, end;    //��ʱ
			start = clock();
			while (1)
			{
				my_recvfrom(recv_packet);
				if (recv_packet.flag == '1' && recv_packet.ACK == send_packet.Seq)
				{
					cout << " Seq=" << send_packet.Seq << " ACK=" << recv_packet.ACK << endl;
					cout << "����" << i << "�ɹ�����" << endl;
					break;
				}

				end = clock();

				//�ش�����������
				if (count == SEND_TIMES)
				{
					cout << "[log]" << get_time() << ">>>" << "�ش�ʧ�ܣ�" << endl;
					break;
				}

				//��ʱ
				if (((end - start) / CLOCKS_PER_SEC) > WAIT_TIME)
				{
					count++;
					start = clock();
					cout << "[log]" << get_time() << ">>>" << "�� " << count << " ���ش�" << endl;
					re_sendto(send_packet);
				}
			}
			//cout << "[log]" << get_time() << ">>>" << "�ش�ʧ��!" << endl;
			//return false;
		}
		else
		{
			send_packet.is_final_file = true;
			//���һ���־λ��λ
			send_packet.len = file_len - i * max_size;
			for (int j = 0; j < send_packet.len; j++)
				send_packet.data[j] = file_buf[i * max_size + j];
			if (send_packet.len != 0)
			{
				//�״η���
				my_sendto(send_packet);

				int count = 0;    //�ش�����
				clock_t start, end;    //��ʱ
				start = clock();
				while (1)
				{
					my_recvfrom(recv_packet);
					if (recv_packet.flag == '1' && recv_packet.ACK == send_packet.Seq)
					{
						cout << " Seq=" << send_packet.Seq << " ACK=" << recv_packet.ACK << endl;
						cout << "����" << i << "�ɹ�����" << endl;
						break;
					}

					end = clock();

					//�ش�����������
					if (count == SEND_TIMES)
					{
						cout << "[log]" << get_time() << ">>>" << "����ʧ�ܣ�" << endl;
						break;
					}

					//��ʱ
					if (((end - start) / CLOCKS_PER_SEC) > WAIT_TIME)
					{
						count++;
						start = clock();
						cout << "[log]" << get_time() << ">>>" << "�� " << count << " ���ش�" << endl;
						re_sendto(send_packet);
					}
				}
				//cout << "[log]" << get_time() << ">>>" << "�ش�ʧ��!" << endl;
				//return false;
			}
		}
	}
}

int main()
{
	int state1, state2;  //����ֺ����ķ���ֵ
	bool state3;

	WORD wVersionRequested = MAKEWORD(2, 2); //������ϣ��ʹ�õ�socket����߰汾
	WSADATA wsaData;  //���õ�Socket����ϸ��Ϣ��ͨ��WSAStartup������ֵ

	//��ʼ��Socket DLL��Э��ʹ�õ�Socket�汾����ʼ���ɹ��򷵻�0������Ϊ�����int������
	state1 = WSAStartup(wVersionRequested, &wsaData);
	if (state1 == 0)
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˳�ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˳�ʼ��Socket DLLʧ�ܣ�" << endl;

	//���÷��Ͷ�socket������
	u_long imode = 1;
	ioctlsocket(sockSend, FIONBIO, &imode);

	//���Ͷ�socket����ַ��
	sockSend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	addrSend.sin_family = AF_INET;  //IPv4��ַ����
	addrSend.sin_addr.s_addr = inet_addr(send_IP);  //���Ͷ�IP
	addrSend.sin_port = htons(send_Port);   //���Ͷ˵Ķ˿ں�
	state2 = bind(sockSend, (SOCKADDR*)&addrSend, sizeof(SOCKADDR));//�󶨵�ַ
	if (state2 == 0)
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˰󶨱��ص�ַ�ɹ�" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "���Ͷ˰󶨱��ص�ַʧ�ܣ�" << endl;

	//������ն˵�ַ
	addrRecv.sin_family = AF_INET;  //IPv4��ַ����
	addrRecv.sin_addr.s_addr = inet_addr(recv_IP);  //���ն�IP
	addrRecv.sin_port = htons(recv_Port);   //���ն˵Ķ˿ں�

	//��������
	state3 = send_Conn();
	if (state3)
	{
		if (read_file())
		{
			stop_wait_send();
		}

		//�Ͽ�����
		send_Clo();
	}
	
	//����ʹ��Socket���ͷ�Socket��Դ
	closesocket(sockSend);
	WSACleanup();

	return 0;
}