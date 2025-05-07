## COMANDI UTILI

    COMPILAZIONE SENZA LIBRERIA ESTERNA
        gcc myFTclient.c -o myFTclient 
        gcc myFTserver.c -o myFTserver 

    CON LIBRERIA
        CREAZIONE LIBRERIA
        gcc -c -fPIC ftp_lib.c -o ftp_lib.o
        gcc -shared -o libftp_lib.so ftp_lib.o
    COMPILAZIONE CON LINK A LIBRERIA
        gcc -o myFTserver myFTserver.c -L. -lftp_lib
        gcc -o myFTclient myFTclient.c -L. -lftp_lib

ss -tulnp | grep 8080 //verifica porta in ascolto


## SERVER USAGE
myFTserver -a server_address -p port -d ft_root_directory
    la DIR ft_root_directory viene creta se non esiste

    ESEMPIO
    ./myFTserver -a 127.0.0.1 -p 8080 -d storage 
        (accetta solo un nome dir senza sottodirectory)


## CLIENT USAGE
# SCRITTURA
myFTclient -w -a server_address -p port -f local_path/filename_local -o remote_path/filename_remote

    ESEMPIO
    ./myFTclient -w -a 127.0.0.1 -p 8080 -f source1.txt -o dir1/source1_remote.txt
        (scrive all'interno della dir del server (storage) il file con un nuovo nome ed all'interno di un subdir dir1 che crea se non esiste)

    ./myFTclient -w -a 127.0.0.1 -p 8080 -f source1.txt
        (se ometto il parametro -o scrive il file sul server con lo stesso nome, sempre all'interno della dir pricipale del server (storage))

Più comandi consecutivi vengono accodati uno dietro l'altro per non creare contese



# LETTURA
myFTclient -r -a server_address -p port -f remote_path/filename_remote -o local_path/filename_local

    ESEMPIO
    ./myFTclient -r -a 127.0.0.1 -p 8080 -f dir1/source1_remote.txt -o testoScaricato.txt


# TEST APPLICAZIONE
    Test attivazione server e creazione directory, storage non esistente (cancellare se esiste)
./myFTserver -a 127.0.0.1 -p 8080 -d storage

    Comando client in WRITE
./myFTclient -w -a 127.0.0.1 -p 8080 -f source1.txt -o dir1/source1_remote.txt

    Comando client WRITE con switch -o mancante (usa lo stesso nome di -f)
./myFTclient -w -a 127.0.0.1 -p 8080 -f source1.txt

    Comando client READ
./myFTclient -r -a 127.0.0.1 -p 8080 -f source1.txt -o source1_from_server.txt
./myFTclient -r -a 127.0.0.1 -p 8080 -f dir1/source1_remote.txt -o dir2/copia_source1_remote.txt 

    Comando client READ con switch -o mancante (usa lo stesso nome di -f)
./myFTclient -r -a 127.0.0.1 -p 8080 -f dir1/source1_remote.txt

    Comando client LIST
./myFTclient -l -a 127.0.0.1 -p 8080 -f dir1
./myFTclient -l -a 127.0.0.1 -p 8080 -f dir55     (dir55 non esistente)

    Testare vari comandi con switch errati o mancanti
./myFTclient -k -a 127.0.0.1 -p 8080 -f source1.txt     <<<<k>>>>
./myFTclient -r -b 127.0.0.1 -p 8080 -f source1.txt     <<<-b>>>

    Testare gestione conflitto su accesso file da diversi thread (client) - VERIFICARE LOGS A TERMINALE
./myFTclient -w -a 127.0.0.1 -p 8080 -f source1.txt -o output.txt
./myFTclient -w -a 127.0.0.1 -p 8080 -f source2.txt -o output.txt
./myFTclient -w -a 127.0.0.1 -p 8080 -f source3.txt -o output.txt

./myFTclient -r -a 127.0.0.1 -p 8080 -f output1.txt -o result.txt
./myFTclient -r -a 127.0.0.1 -p 8080 -f output2.txt -o result.txt
./myFTclient -r -a 127.0.0.1 -p 8080 -f output3.txt -o result.txt




### REQUISISTI

Lo studente deve realizzare un applicazione client-server che permette di trasferire file da un client and un server (scrittura di un file) e, viceversa, da un server ad un client (lettura di un file).

L'applicazione è composta da due programmi, un client che effettua richieste di lettura e scrittura di un file, un server che riceve le richieste e gestisce le richieste dei client.

Assumendo che l'applicazione server si chiama myFTserver e l'applicazione client si chiama myFTclient il comportamento deve essere il seguente.

Server
Il comando
myFTserver -a server_address -p server_port -d ft_root_directory

esegue il programma server mettendolo in ascolto su di un determinato indirizzo IP e porta ed indicando la directory nella quale andare a scrivere/leggere i file. Se ft_root_directory non esiste deve essere creata.

Una volta in esecuzione, il server deve accettare connessioni da uno o piu' client e gestirle concorrentemente.
Richieste di scrittura concorrenti sullo stesso file devono essere opportunamente gestite (come la richiesta di creazione concorrente di path con lo stesso nome).
Il programma server deve gestire tutte le eccezioni come ad esempio: richiesta di accesso a file non esistente (per la lettura), errore nel binding su IP e porta, parametri di invocazione del comando errati o mancanti, spazio su disco esaurito, interruzione della connessione con il client.

Client
Il comando 
myFTclient -w -a server_address -p port  -f local_path/filename_local -o remote_path/filename_remote

esegue il programma client, crea una connessione con il server specificato da server_address:port, e scrive il file local_path/filename_local sul server con nome filename_remote e nella directory specificata da remote_path. remote_path avra' root nella directory del server specificata con ft_root_directory

il comando
myFTclient -w -a server_address -p port  -f local_path/filename_local

si comporta come il precedente ma il nome del path remoto e del file remoto sono gli stessi del path e file locale.

il comando 
myFTclient -r -a server_address -p port  -f remote_path/filename_remote -o local_path/filename_local

esegue il programma client, crea una connessione con il server specificato da server_address:port e legge il file specficato da remote_path/filename_remote trasferendolo al programma client che lo scrivera' nella directory local_path assegnando il nome filename_local

il comando
myFTclient -r -a server_address -p port  -f remote_path/filename_remote

si comporta come il precedente ma il nome del path locale e del file locale sono gli stessi del path e file remoto.

il comando
myFTclient -l -a server_address -p port  -f remote_path/

permette al client di ottenere la lista dei file che si trovano in remote_path (effettua sostanzialmente un ls -la remoto). Ia lista dei file deve essere visualizzata sullo standard output del terminale da cui viene eseguito il programma myFTclient.

Il programma client deve gestire tutte le eccezioni del caso. come ad esempio: parametri di input errati, file remoto non esistente (lettura), spazio di archiviazione insufficiente sul server (scrittura) e sul client, interruzione della connessione con il server

Valutazione:
Il codice sottomesso deve essere commentato in modo chiaro ed estensivo
il codice deve essere accompagnato da una relazione (2 pagine massimo) che descriva le scelte operate.
In sede di verifica, la studentessa/lo studente deve dimostrare che il codice funzioni
 
La consegna deve avvenire come segue:
Sottomettere un archivio .tgz o .zip, nominato come MATRICOLA_HW2 contenente i file .h, .c e .pdf (per la relazione)