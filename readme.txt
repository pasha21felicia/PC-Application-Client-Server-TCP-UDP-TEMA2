DIGORI PARASCOVIA - 323CC - TEMA 2 - PROTOCOALE DE COMUNICATII
Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

Mod de rulare:
make
./server {port}
./subscriber {id} {ip} {port}

Uneori pe checker nu se executa testele daca nu ati rulat si manual serverul, pt ca makefile-ul
imi face doar fisiere .o nu si executabile.

Am pornit aceasta tema de la skeletul din laboratorul 8, deaceea nici nu am prea schimbat
mult numele variabilelor sau strctura de baza din server si subscriber.

Subscriber:
Am adaptat skeletul la cerintele temei, verificand primirea corecta a parametrilor
cu ajutorul programarii defensive. Dezactivez algoritmul lui Neagle si transmit catre server
id-ul clientului curent.
Pe parcursul activitatii subscriber-ului verific tipul de mesaje primite de la stdout
daca este exit, inchid socketul curent si finalizez activitatea clientului.
Daca comanda contine cuvantul 'subscribe'/'unsubscribe' verific din nou cu ajutorul 
programarii defensive daca am primit toti parametrii corect si trimit un mesaj de rigoare 
in stderr daca este cazul utilizand functia check_errors(condition, assertion); 
Datele citite le mapez pe structura unui mesaj de tip msg_t (care contine informatia despre 
topic, SF, command_type). Trimit mesajul de abonare/dezabonare la server si ulterior primesc 
raspunsul de "Subscribed to topic" sau de "Unsubscribed from topic".

Mai am cazul cand primesc mesaje de la server, cu informatii de la clientul UDP. 
Aceste informatii sunt mapate pe o structura de tip tcp_msg_t, care afiseaza ulterior in subscriber
datele de ip, udp_port, topic, type, content.

Server:
Adaug in skelet parametrii de initializare a socket-ului de UDP, avand ca model
laboratorul 6, verific parametrii conform programarii defensive si dezactivez alg.
lui Neagle. 
Pentru a tine evidenta subscriber-ilor folosesc 2 unordered_map-uri, unul in care tin clientii
activi si socket-ul lor, si un map pentru toti clientii care au fost vreodata conectati in Aplicatie
si id-ul lor. 
In activitatea server-ului verific daca primesc o comanda de la stdout "exit" ca sa inchid toti
socketii. 
Daca a venit o cerere de conexiune de la un socket inactiv tcp, serverul o accepta si primeste id-ul
subscriber-ului in buffer. Verific lista de clienti activi daca mai exista un client activ cu 
acelasi id, daca exista acelasi id afisez mesajul 'Client is already connected', setez flag-ul
different_clients pe false si inschid socketul acestui nou client.

Asigurandu-ma ca clientul este valid populez structura client_t pentru un client current
cu informatiile acestuia. Daca clientul cu acest id a mai fost candva conectat si se afla 
in allClients updatez lista de topicuri a noului client creat, copiindu-le din clientul cu acelasi id
din allClients si trimit mesajele bufferate catre acest client daca exista.
Inserex noul client in lista de activeClients si daca nu a mai fosr connectat si in allClients.
Trimit mesaj de confirmare in server ca "New client connected".

Daca a venit un mesaj de la UDP il receptionez si mapez pe structura de tip udp_msg_t.
Prin functia de parsare create_tcp_message_from_udp() populez strctura tcp_msg_t cu datele
pregatite pentru a le transmite clientului.
Adaug mesajul in bufferedMessages a tuturor clientilor carfe au fost conectati candva si au avut 
SF = 1. Apoi transmit mesajul catre toti clientii activi aboanti la acel topic.


In final, daca s-au primit date pe unul din socketii de client, serverul trebuie sa le 
receptioneze, vede daca clientul s-a deconectat sau a trimit o cererede subscribe/unsubscribe.
Daca este o cere de abonare atunci adaug topicul in toti vectorii de mesaje al subscriber-ului
si daca este unsubscribe ii sterg topicul de peste tot, atat din prezenta sa in activeClients
cat si in allClients.

Tema mi s-a parut acceptabila ca dificultate totusi sincer enuntul nu este foarte clar in partea
de descriere a functionalitatii aplicatiei, adica faptul ca daca deconectezi un client si el trebuie
sa ramana abonat la aceleasi topicuri la reconectare este scrisa foarte subtil:)) la final de enunt.
Ar fi ajutat si un exemplu de rulare, gen care este flow-ul transmiterii mesajelor.
In rest este o tema draguta, simt ca am invatat destul si am avut si multa libertate in modul cum 
sa imi gandesc codul.

Pentru coding style am incercat sa scriu in 80 caractere pe linie si am folosit optiunea de beautify
din VS Code.




