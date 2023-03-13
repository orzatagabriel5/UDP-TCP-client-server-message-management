#include "helpers.h"


// Functie care printeaza mesajele venite de la server
void receive_message(udp_msg msg)
{
    char sign;

    switch (msg.data_type)
    {
    case STRING:
    {
        msg.content[1500] = '\0';
        printf("%s - %s - %s\n", msg.topic, "STRING", msg.content);
        break;
    }
    case INT:
    {
        // Citesc octetul de semn
        sign = msg.content[0];

        uint32_t int_message = ntohl(*(uint32_t*)(msg.content + 1));
        // Daca numarul este negativ
        if(sign == 1){ 
            int_message *= -1; 
        }

        printf("%s - %s - %d\n", msg.topic, "INT", int_message);
        
        break;
    }
    case SHORT_REAL:
    {
        float short_message = ((float)ntohs(*(uint16_t*)(msg.content))) / 100;
        printf("%s - %s - %.2f\n", msg.topic, "SHORT_REAL", short_message);
        
        break;
    }
    case FLOAT:
    {
        sign = msg.content[0];
        float float_message = ntohl(*(uint32_t*)(msg.content + 1));
        uint8_t power = *(uint8_t *)(msg.content + 5);
        // Ridic la putere si schimb semnul daca e cazul
        if(sign == 1){
            float_message *= (-1);
            if(power != 0){
                float_message = float_message * pow(0.1, power);
            }
        } else {
            if(power != 0){
                float_message = float_message * pow(0.1, power);
            }
        }

        printf("%s - %s - %.f\n", msg.topic, "FLOAT", float_message);

        break;
    }
    default:
        printf("Wrong data_type\n");
        break;
    }
}

// Functie care converteste string-ul citit in int
int set_command(char* command)
{
    if(strcmp(command, "exit") == 0){
        return 3;
    } else if (strcmp(command, "subscribe") == 0) {
        return 1;
    } else if (strcmp(command, "unsubscribe") == 0) {
        return 0;
    }
    return -1;
}

// Functie care trimite mesaje catre server
void send_command(int socket, char *argv[])
{
    struct tcp_msg msg;
    char command_type[12];
    int ret;
    memset(&msg, 0, sizeof(struct tcp_msg));
    strcpy(msg.id, argv[1]);
    cin >> command_type;

    msg.command_type = set_command(command_type);

    switch(msg.command_type)
    {
        case 3:
            // Clientul da disconnect
            ret = send(socket, &msg, sizeof(struct tcp_msg), 0);
            DIE(ret < 0, "exit error");
			close(socket);
            exit(0);
        case 1:
            // Cand clientul da subscribe la un topic
            scanf("%s", msg.topic);
            int SF;
            cin >> SF;
            msg.SF = SF;
            
            ret = send(socket, &msg, sizeof(struct tcp_msg), 0);
            DIE(ret < 0, "subscribe error");

            // Pentru cazul in care serverul a fost inchis
            if (ret == 0) {
                // Inchid socketul si clientul
                close(socket);
                return;
            }
            printf("Subscribed to topic.\n");
            break;
        case 0:
            // Cand clientul da unsubscribe la un topic
            cin >> msg.topic;
            ret = send(socket, &msg, sizeof(struct tcp_msg), 0);
            DIE(ret < 0, "unsubscribe error");
            if (ret == 0) {
                close(socket);
                return;
            }
            printf("Unubscribed from topic.\n");
            break;

        default:
            printf("Wrong command\n");
            break;
    }
}


void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int client_socket, ret;
	struct sockaddr_in serv_addr;
    struct tcp_msg tcp_msg;
    struct udp_msg udp_msg;
    fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar

	if (argc < 3) {
		usage(argv[0]);
	}
    // Pornesc socket-ul clinetului
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(client_socket < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

    // Creez conexiunea la server
	ret = connect(client_socket, (struct sockaddr*) &serv_addr,
                    sizeof(serv_addr));
	DIE(ret < 0, "connect");

    // Imediat dupa trimit id-ul catre server pentru a se verifica
    // Daca este deja conectat un client cu acelasi id, clientul
    // va fi deconectat de la server.

    memset(&tcp_msg, 0, sizeof(struct tcp_msg));
	tcp_msg.command_type = SEND_ID; // send id
    strcpy(tcp_msg.id, argv[1]);
	int send_ret = send(client_socket, &tcp_msg, sizeof(struct tcp_msg), 0);
	DIE(send_ret < 0, "send error");


    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    FD_SET(client_socket, &read_fds); // adaug clientul in multimea
                                      // de descriptori
	FD_SET(0, &read_fds); // 0 este pentru citirea de la tastatura


	while (1) {

  		tmp_fds = read_fds;

        int select_response = select(client_socket + 1, &tmp_fds,
                                NULL, NULL, NULL);
		DIE(select_response < 0, "select error");

  		// Verific daca se primeste un mesaj de la tastatura
        if (FD_ISSET(0, &tmp_fds)){
            send_command(client_socket, argv);

        } else if (FD_ISSET(client_socket, &tmp_fds)) {
            // Daca se primeste un mesaj de la server
            memset(&udp_msg, 0, sizeof(struct udp_msg));
            int bytes_recv = recv(client_socket, &udp_msg,
                            sizeof(struct udp_msg), 0);
			DIE(bytes_recv < 0, "recv error");
            if (bytes_recv == 0){
                close(client_socket);
                return 0; // serverul este inchis
            }
            receive_message(udp_msg);
        }
	}

	return 0;
}
