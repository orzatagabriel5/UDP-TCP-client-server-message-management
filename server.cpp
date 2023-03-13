#include "helpers.h"


void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

// Functie care executa comenzile venite de la client
void do_command(int socket, struct tcp_client &client, fd_set &read_fds,
                map<string, unordered_set<struct tcp_client*>> &topic_clients,
                map<string, int> &connected_clients, struct tcp_msg tcp_msg,
                struct sockaddr_in cli_addr, map<string,
                list<struct udp_msg>> &msg_to_send)
{
    strcpy(client.id, tcp_msg.id);

    switch (tcp_msg.command_type)
    {
    case DISCONNECT:
    {
        // Un client se deconecteaza
        close(socket);
        FD_CLR(socket, &read_fds);
        connected_clients[tcp_msg.id] = 0;
        printf("Client %s disconnected.\n", tcp_msg.id);

        break;
    }
    case SEND_ID:
    {
        // Dupa ce un client se conecteaza la server va trimite mereu
        // ca pachet urmator, un mesaj cu id-ul sau, deci variabila
        // cli_addr va fi mereu cea corespunzatoare

        if(connected_clients[tcp_msg.id] == 1) {
            printf("Client %s already connected.\n", tcp_msg.id);
            close(socket);
            FD_CLR(socket, &read_fds);
            break;
        }
        printf("New client %s connected from %s:%d.\n", tcp_msg.id,
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        strcpy(client.id, tcp_msg.id);
        connected_clients[client.id] = 1;
        // Daca clientul se reconecteaza la server si are campul SF 1,
        // atunci ii trimit toate mesajele de la topicurile unde este
        // abonat

        if(msg_to_send.find(client.id) != msg_to_send.end()) {
            while(msg_to_send[client.id].empty() == false) {
                int ret = send(socket, &msg_to_send[client.id].back(),
                                 sizeof(struct udp_msg), 0);
                DIE(ret < 0, "SF error");
                msg_to_send[client.id].pop_back();
            }
        }

        break;
    }
    case SUBSCRIBE:
    {
        // Daca topicul la care vrea sa se conecteze exista deja il
        // adaug si pe el in set-ul de clienti care sunt abonati la el.
        // In caz contrat il creez si el devine primul client abonat.
        client.SF = tcp_msg.SF;
        client.socket_number = socket;
        if(topic_clients.find(tcp_msg.topic) == topic_clients.end()) {
            unordered_set<struct tcp_client*> new_set;
            new_set.insert(&client);
            tcp_msg.topic[50] = 0;
            topic_clients.insert({tcp_msg.topic, new_set});
        } else {
            topic_clients[tcp_msg.topic].insert(&client);
        }
        break;
    }
    case UNSUBSCRIBE:
    {
        // Prima data verific daca topicul de la care vrea sa se dezaboneze
        // exista, apoi il sterg din map-ul de clienti contectati la el

        for(auto pair: topic_clients){
            for(auto it : pair.second){
                if(strcmp(it->id, tcp_msg.id) == 0){
                    delete(it);
                    topic_clients[tcp_msg.topic].erase(it);
                }
            }
        }
        break;
    }
    default:
    {
        printf("Wrong command\n");
        break;
    }
    }
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int tcp_socket, new_tcp_socket, udp_socket, portno;
	struct sockaddr_in serv_addr, cli_addr;
	int i, tcp_ret, udp_ret, optval = 1;
	socklen_t socket_len;
    struct udp_msg udp_msg;
    struct tcp_msg tcp_msg;

    map<string, int> connected_clients;
    map<string, list<struct udp_msg>> msg_to_send;
    map<string, unordered_set<struct tcp_client*>> topic_clients;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds)
    // si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    // Creez un socket TCP pentru a receptiona conexiunile
	tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket < 0, "tcp_socket error");

    // Creez un socket UDP pentru a receptiona mesajele de la clientii udp
    udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "udp_socket error");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Asociez adresa serverului cu socketul tcp creeat si ascult
    // prin el conexiunile de la clientii tcp
    
	tcp_ret = bind(tcp_socket, (struct sockaddr *) &serv_addr,
                    sizeof(struct sockaddr));
	DIE(tcp_ret < 0, "tcp_bind");

    // Asociez adresa serverului cu socketul udp creeat
    udp_ret = bind(udp_socket, (struct sockaddr *) &serv_addr,
                    sizeof(struct sockaddr));
	DIE(udp_ret < 0, "udp_bind");

	tcp_ret = listen(tcp_socket, MAX_CLIENTS);
	DIE(tcp_ret < 0, "listen");

	// Adaug noul file descriptor (socketul pe care se asculta conexiuni)
    // in multimea read_fds
	FD_SET(tcp_socket, &read_fds);
	fdmax = tcp_socket;

    // Setez Neagle-ul pasiv
    int result = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, &optval,
                    sizeof(int));
    DIE(result < 0, "Neagle error");


    FD_SET(udp_socket, &read_fds);
    if(udp_ret > fdmax) {
        fdmax = udp_socket;
    }

    // Adaug descriptorul pentru citirea de la tastatura in
    // multimea de descriptori
    FD_SET(STDIN_FILENO, &read_fds);

	while (1) {
		tmp_fds = read_fds; 
		tcp_ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(tcp_ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcp_socket) {
					// A venit o cerere de conexiune pe socketul inactiv
                    // (cel cu listen), pe care serverul o accepta
					socket_len = sizeof(cli_addr);
					new_tcp_socket = accept(tcp_socket,
                        (struct sockaddr *) &cli_addr, &socket_len);
					DIE(new_tcp_socket < 0, "accept error");

                    // Setez Neagle-ul pasiv
                    int result = setsockopt(tcp_socket, IPPROTO_TCP,
                                    TCP_NODELAY, &optval, sizeof(int));
                    DIE(result < 0, "Neagle error");

					// se adauga noul socket intors de accept() la multimea
                    // descriptorilor de citire
					FD_SET(new_tcp_socket, &read_fds);
					if (new_tcp_socket > fdmax) { 
						fdmax = new_tcp_socket;
					}                    
				} else if (i == udp_socket) {
                    
                    // Primesc mesaje de la un client UDP
                    memset(&udp_msg, 0, sizeof(struct udp_msg));
					socket_len = sizeof(cli_addr);

                    int recv = recvfrom(i, &udp_msg, sizeof(struct udp_msg),
                            0, (struct sockaddr *) &cli_addr, &socket_len);
                    DIE(recv < 0, "recvfrom_udp error");

                    udp_msg.content[1500] = '\0';

                    for(auto it = topic_clients[udp_msg.topic].begin();
                        it != topic_clients[udp_msg.topic].end(); it++){
                        if(connected_clients[(*it)->id] == 1) {
                            // Trimit mesajul udp clientilor conectati
                            // la server si abonati la topic
                            udp_msg.ip_address = cli_addr.sin_addr;
                            udp_msg.port = htons(cli_addr.sin_port);

                            int ret = send((*it)->socket_number, &udp_msg,
                                            sizeof(struct udp_msg), 0);
                            DIE(ret < 0, "send error");

                            if(ret == 0){

                                close((*it)->socket_number);
                                FD_CLR((*it)->socket_number, &read_fds);
                                connected_clients[(*it)->id] = 0;

                                printf("Client %s disconnected.\n", (*it)->id);
                            }
                        } else if(connected_clients[(*it)->id] ==
                                             0 && (*it)->SF == 1){
                            // Salvez mesajul pentru clientii deconectati care
                            // s-au abonat la topic cu SF = 1
                            
                            udp_msg.ip_address = cli_addr.sin_addr;
                            udp_msg.port = htons(cli_addr.sin_port);
                            msg_to_send[(*it)->id].push_back(udp_msg);
                        }
                    }
                } else if (i == 0) {
                    // Mesaj de la tastatura
                    char msg[50];
                    scanf("%s", msg);

                    if(strcmp(msg, "exit") == 0){
                        for(int k = 0; k <= fdmax; k++){
                            close(k);
                        }
                        return 0;
                        // Inchid serverul
                    } else {
                        continue;
                    }
                
                } else {
					// s-au primit date pe unul din socketii de client tcp
					memset(&tcp_msg, 0, sizeof(struct tcp_msg));
					int rec = recv(i, &tcp_msg, sizeof(struct tcp_msg), 0);
					DIE(rec < 0, "recv");

                    struct tcp_client *tcp_client = new struct tcp_client;

                    do_command(i, *tcp_client, read_fds, topic_clients, 
                        connected_clients, tcp_msg, cli_addr, msg_to_send);
				}
			}
		}
	}

	close(tcp_socket);
	return 0;
}
