Orzata Gabriel-Nicusor
Tema 2

Folosesc template-ul de la laboratorul 8 pentru a conecta clientii TCP
la server si pentru a porni serverul.

Clientul TCP
1. Primeste mesaj de la tastatura:
    Am facut o functie care trimite un mesaj tcp catre server care contine
    o comanda de tipul: subscribe, unsubscribe, send_id, exit.
    Am o fucntie care imi converteste string-ul citit de la tastatura
    in int-uri specifice comenzilor si intr-un switch citest(daca este cazul)
    de la tastura parametrii comenzii si trimit mesajul catre server.
2. Primeste mesaj de la server:
    Dau recv intr-o structura udp_msg si apelez functia receive_message.
    In functie de tipul mesajul primit prelucrez datele din payload si
    printez corespunzator.


Serverul
1. Cand un client vrea sa se conecteze serverul da mereu accept si il adauga
    in multimea de descriptori. Imediat dupa, primeste un pachet de la acel
    client cu id-ul lui si verifica daca acesta este deja conectat sau nu.
    Daca a fost abonat la topicuri cu SF=1 ii trimite mesajele salvate acum.
2. Cand un client da subscribe la un topic serverul creeaza topicul daca nu
    exista deja si salveaza perechea {topic, lista de clienti abonati la el}
    intr-un map.
3. Cand un client vrea sa se dezaboneze de la un topic sterg clientul din
    map-ul mentionat mai sus.
4. Cand un client se deconecteaza inchid socketul sau, il scot din multimea
    de descriptori, setez campul sau din map-ul de clienti conectati la 0.

5. Cand primesc un mesaj udp il salvez intr-o structura udp_msg si il trimit
    tuturor clientilor conectati la server si abonati la topicul sau, apoi
    salvez mesajul intr-un map pentru clientii care sunt deconectati, dar au
    campul SF = 1 la abonamentul facut la topicul respectiv.

6. Cand primesc un mesaj de la tastatura tratez doar cazul in care acesta este
    un mesaj de exit, caz in care inchid serverul. Daca este altceva inseamna
    ca am primit un mesaj de la un client tcp. Atunci salvez mesajul intr-o
    structura tcp_msg si o dau ca parametru functiei do_command care trateaza
    subpunctele 1, 2, 3, 4 mentionate mai sus.

PS: Am creeat o biblioteca unde am definit macro-uri pentru dimensiuni
    si strucuturile unde stochez mesajele primite