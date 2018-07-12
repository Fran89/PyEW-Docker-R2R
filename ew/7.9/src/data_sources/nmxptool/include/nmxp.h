/*! \file
 *
 * \brief Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp.h 4965 2012-07-22 07:12:34Z quintiliani $
 *
 */

/*! \mainpage libnmxp and nmxptool: open-source software for Nanometrics data acquisition
 *
 * <center>
 * Matteo Quintiliani
 *
 * <i>
 * Istituto Nazionale di Geofisica e Vulcanologia<br />
 * Centro Nazionale Terremoti
 * </i>
 * 
 * e-mail: quintiliani@ingv.it
 * </center>
 *
 * Documentation is available in the following languages:
 *
 * - \subpage page_english_version English Version
 * - \subpage page_versione_italiana Versione Italiana
 *
 */

/*! \page page_english_version libnmxp and nmxptool: open-source software for Nanometrics data acquisition
 *
 * <center>
 * Matteo Quintiliani
 * 
 * <i>
 * Istituto Nazionale di Geofisica e Vulcanologia<br />
 * Centro Nazionale Terremoti
 * </i>
 * 
 * e-mail: quintiliani@ingv.it
 * </center>
 *
 * \todo
 * 	- Extend english version
 *
 * \section intro_sec Introduction
 *
 * This is the documentation for the <i>APIs</i> that implement the <tt>Nanometrics Protocols</tt>.
 * They have been developed for interacting with \c NaqsServer and \c DataServer.
 *
 * The Nanometrics \c NaqsServer provides online access to time-series, serial data, triggers, and state-of-health data via TCP subscription.
 * 
 * The Nanometrics \c DataServer provides local and remote access to nanometrics, serial, and state-of-health data via TCP/IP.
 *
 * The library offers APIs to:
 * \li interact with \c NaqsServer that uses version 1.4 of the <i>Private Data Stream Protocol</i>
 * \li interact with \c DataServer that uses version 1.0 of the <i>Nanometrics Data Access Protocol</i>
 * \li manage Nanometrics data formats
 * \li request, receive and interpret online and offline data
 *
 * moreover, you can use them to develop software to:
 * \li analyze data in realtime (waveforms, triggers, ...)
 * \li retrieve and convert on the fly data into the mini-SEED records (optional)
 * \li feed SeedLink server (optional)
 * \li feed Earthworm system (optional)
 *
 *
 * \section dependencies_sec Dependencies
 *
 * An optional library is needed to allow \c libnmxp to save mini-SEED records:
 *
 * \li \c libmseed: http://www.iris.edu/manuals/\n
 * The Mini-SEED library. A C library framework for manipulating and managing SEED data records.\n
 * Author: Chad Trabant, <i>IRIS DMC</i>\n
 *
 *
 * \section install_sec Installation
 *
 * \c nmxp library has been developed using <i>GNU Build Tools</i> (\c automake, \c autoconf and \c configure script)
 * taking in account the POSIX Cross-Platform aspects.
 * So you should be able to compile and install it everywhere you can launch the following commands:
 *
 * <tt>./configure</tt>
 * 
 * <tt>make</tt>
 *
 * <tt>make install</tt>
 *
 * Please, refer to the file \b README and \b INSTALL for more details.
 *
 *
 * \section tools_sec Tools
 *
 * Inside the distribution is available a tool which is a client that interact with \c NaqsServer and \c DataServer.
 *
 * \c nmxptool:
 * 	\li implements the <i>Nanometrics Private Data Stream Protocol 1.4</i> and permits to retrieve data in near-realtime.\n
 * 	\li implements the <i>Nanometrics Data Access Protocol 1.0</i> and permits to retrieve backward data.\n
 * Please, refer to the \b README file or help <tt>nmxptool --help</tt>.
 *
 *
 * \subsection examples_sec Examples
 *
 * etc...
 *
 *
 * \section license_sec License
 *
 * http://www.gnu.org/licenses/gpl.html
 *
 * \section about_sec About
 *
 * Matteo Quintiliani - <i>Istituto Nazionale di Geofisica e Vulcanologia</i> - Italy<br />
 * Mail bug reports and suggestions to <quintiliani@ingv.it>
 */
 
 /*! \page page_versione_italiana libnmxp e nmxptool: software open-source per trasmissioni dati sismici Nanometrics
 *
 * <center>
 * Matteo Quintiliani
 * 
 * <i>
 * Istituto Nazionale di Geofisica e Vulcanologia<br />
 * Centro Nazionale Terremoti
 * </i>
 * 
 * e-mail: quintiliani@ingv.it
 * </center>
 * 
 * 
 * \section introduzione_sec Introduzione
 * 
 * Il presente documento descrive le modalit&agrave; di impiego della libreria
 * software progettata dall'autore al fine di implementare i protocolli di
 * trasmissione Nanometrics. Lo sviluppo di tale libreria nasce
 * principalmente dall'esigenza all'interno dell'INGV di gestire un numero
 * sempre pi&ugrave; crescente di canali sismici acquisiti tramite sistema
 * Nanometrics. La libreria denominata libnmxp offre un insieme di APIs
 * (Application Program Interface) ben documentate che permettono di
 * sviluppare software capace di interagire con i due tipi di server
 * Nanometrics:
 * 
 * \li <i>NaqsServer</i> il quale implementa il protocollo per trasmissioni di dati in
 * tempo reale;
 * \li <i>DataServer</i> il quale implementa il protocollo per il recupero di dati
 * archiviati.
 * 
 * Insieme alla libreria viene inoltre distribuito un programma chiamato
 * nmxptool che basandosi su di essa, permette di eseguire interrogazioni,
 * ricevere dati in tempo reale e/o off-line, ed inoltre permette di
 * salvare questi ultimi in diversi formati, quali NMX e mini-SEED. Tale
 * programma pu&ograve; inoltre essere utilizzato come modulo per il sistema
 * Earthworm o come plug-in per server SeedLink.
 * 
 * Uno dei principali contributi offerti da questo sviluppo consiste nella
 * possibilita di gestire connessioni di tipo Raw Stream con riordinamento
 * dei pacchetti ritrasmessi: ci&ograve; permette di garantire un buon compromesso
 * fra la continuit&agrave; del dato e una bassa latenza.
 * 
 * L'intero sviluppo si &egrave; basato sul manuale del corso Nanometrics
 * [Nanometrics, Inc., 1989-2002], in particolare su Nanometrics Data
 * Formats, Reference Guide inclusa nella sezione Software Reference
 * Manuals.
 * 
 * La libreria libnmxp e il programma nmxptool sono scritti in linguaggio C
 * e sviluppati usando i GNU Build Tools (automake, autoconf, e script
 * configure) tenendo in considerazione gli aspetti di compilazione
 * trasversale (cross-compilation) su tutte le piattaforme di tipo
 * POSIX/UNIX. I sorgenti sono gratuiti e possono essere modificati e
 * ridistribuiti sotto i termini  GNU Library General Public License,
 * ulteriori informazioni possono essere trovate su http://www.gnu.org/.
 * 
 *
 * \section protcolli_sec Protocolli Nanometrics
 * 
 * Prendendo ad esempio una configurazione tipica del flusso trasmissivo
 * dei dati da una stazione sismica, acquisita attraverso i servers
 * Nanometrics, fino a raggiungere i processi di acquisizione e
 * localizzazione situati nella Sala di Monitoraggio dell'INGV, come
 * mostrato in figura 1 possiamo suddividere tale flusso in due parti:
 * 
 * \li <i>Stazione Sismica - Nanometrics Servers</i>: i dati ricevuti dalla porta
 * seriale di uno strumento vengono  convertiti nel formato NMXP e poi
 * spediti in pacchetti UDP ai servers di acquisizione.
 * \li <i>Nanometrics Servers - Clients</i>: applicazioni software si connettono ai
 * servers Nanometrics per recuperare dati in tempo reale o in differita,
 * ricevere informazioni sullo ``stato di salute'' (state-of-health) degli
 * strumenti, triggers o eventi sismici.
 * 
 * 
 * \image html doc/images/stazione_nanometrics_servers.jpg "Figura 1. Configurazione tipica di un flusso trasmissivo dei dati da una stazione sismica ai processi di acquisizione e localizzazioni situatinella Sala di Monitoraggio dell'INGV."
 * 
 * 
 * 
 * Ci&ograve; di cui terremo conto in questo documento fara riferimento
 * principalmente al lato trasmissivo Nanometrics Servers - Clients ed in
 * particolare alle specifiche dei protocolli:
 * 
 * \li <i>Private Data Stream versione 1.4</i>, il quale definisce il protocollo di
 * comunicazione di un client con un NaqsServer.
 * \li <i>Data Access Protocol versione 1.0</i>, il quale definisce il protocollo di
 * comunicazione con un DataServer.
 * 
 * La differenza significativa fra i due protocolli i che ad un NaqsServer
 * ci si connette per ricevere dati, informazioni sui canali, triggers e
 * eventi in tempo reale (online) mentre ad un DataServer ci si connette
 * per accedere ai dati e alle informazioni del passato (offline).
 * 
 * Di seguito, senza alcuna pretesa di completezza, vengono descritti i
 * comportamenti generali di questi due tipi di servers e dei rispettivi
 * protocolli di trasmissione. Per maggiori dettagli fare riferimento al
 * manuale del corso Nanometrics [Nanometrics, Inc., 1989-2002], in
 * particolare Nanometrics Data Formats, Reference Guide inclusa nella
 * sezione Software Reference Manuals.
 * 
 *
 * \subsection pds_sec Private Data Stream 
 * 
 * Il NaqsServer fornisce accesso online via TCP/IP a dati di tipo
 * time-series, serial data, triggers, e state-of-health.  Il sottosistema
 * del NaqsServer chiamato Stream Manager si comporta come un DataServer:
 * accetta connessioni e richieste di dati da programmi client e redirige i
 * dati richiesti ad ogni programma client in tempo quasi reale. I dati
 * compressi possono essere richiesti in due modi:
 * 
 * \li <i>Raw stream</i>: tutti i pacchetti (sia quelli originali quelli ritrasmessi)
 * sono rediretti nello stesso ordine nel quale sono ricevuti. I pacchetti
 * possono essere persi, duplicati, ritrasmessi, non ordinati, ma con
 * minimo ritardo.
 * \li <i>Buffered stream</i>: anche chiamato Short-term-complete data stream. Per
 * ogni canale viene garantito l'ordine cronologico dei pacchetti ma
 * possono riscontrarsi piccoli gaps temporali ogni qualvolta si verifichi
 * una ritrasmissione di un pacchetto dal lato trasmissivo
 * Stazione Sismica - Nanometrics Server.
 * 
 * Ogni programma che abbia necessita di interagire con un NaqsServer deve
 * implementare il protocollo di comunicazione Private Data Stream versione
 * 1.4 cos&igrave; come descritto schematicamente di seguito:
 * 
 * \li Aprire un socket su Stream Manager. La porta di default &egrave; la 28000.
 * \li Inviare un messaggio di tipo Connect allo Stream Manager.
 * \li Ricevere e gestire il messaggio di tipo ChannelList dallo Stream
 * Manager.
 * \li (opzionale) Inviare un messaggio di RequestPending finchy la richiesta
 * non &egrave; pronta. Il server attende al massimo 30 secondi, dopodichy chiude
 * la connessione se non ha ricevuto ny una Request Pending, ny un
 * messaggio di AddChannels.
 * \li Inviare un messaggio di tipo AddChannels allo Stream Manager.
 * \li Ricevere fino alla fine i pacchetti dallo Stream Manager ed elaborarli.
 * Opzionalmente inviare nuovi messaggi AddChannels (passo 5) oppure
 * RemoveChannels. Il client deve saper gestire i messaggi di tipo Error.
 * \li Inviare un messaggio di tipo TerminateSubscription allo Stream Manager.
 * \li Chiudere il socket.
 * 
 * 
 * \subsection dap_sec Data Access Protocol
 * 
 * Il DataServer fornisce accesso locale e remoto via TCP/IP a dati di tipo
 * time-series, serial data, triggers e state-of-health. Il DataServer
 * fornisce inoltre informazioni sulla disponibilita dei dati di ogni
 * canale per mezzo di due tipi di liste:
 * 
 * \li <i>Channel List</i>: una lista dei canali disponibili.
 * \li <i>Precis List</i>: una lista dei canali disponibili inclusi i tempi di inizio
 * e di fine dei dati disponibili per ogni canale.
 * 
 * Ogni programma che abbia necessita di interagire con un DataServer deve
 * implementare il protocollo di comunicazione Data Access Protocol
 * versione 1.0 cos&igrave; come descritto schematicamente di seguito:
 * 
 * \li Aprire un socket sul DataServer. La porta di default &egrave; la 28002.
 * \li Ricevere come intero di 4 byte rappresentante il tempo della
 * connnessione sul DataServer.
 * \li Inviare un messaggio di tipo ConnectRequest includendo il tempo di
 * connessione ricevuto precedentemente.
 * \li Aspettare un messaggio di tipo Ready dal DataServer.
 * \li Inviare un messaggio di tipo Request al DataServer.
 * \li Ricevere ed elaborare i dati di risposta finchy non si riceve il
 * messaggio di tipo Ready. Il messaggio Ready indica che il server i
 * pronto per ricevere nuove richieste.
 * \li Ripetere i passi 5 e 6 per ogni richiesta.
 * \li (opzionale) Inviare un messaggio di tipo Terminate al DataServer.
 * \li Chiudere il socket.
 *
 * 
 * \section libnmxp_sec La libreria libnmxp
 * 
 * Dopo aver descritto in generale i protocolli di comunicazione
 * Nanometrics passiamo ora ad illustrare come la libreria &egrave; organizzata e
 * quali sono le strutture dati e le funzioni che espone per il loro
 * utilizzo nello sviluppo di un programma che debba interagire con un
 * NaqsServer, un DataServer o entrambi.
 * 
 * La libreria &egrave; stata scritta in linguaggio C con una strutturazione a
 * livelli dei sorgenti.
 * 
 * Le APIs (Application Program Interface) che compongono la libreria
 * offrono principalmente funzionalita a livello applicativo per lo
 * sviluppo di software che implementi i protocolli Private Data Stream 1.4
 * e Data Access Protocol 1.0.
 * 
 * Esse sono state concepite nell'ottica della realizzazione di programmi
 * in grado di:
 * 
 * \li manipolare i dati di tipo Nanometrics;
 * \li richiedere, ricevere ed interpretare i dati online e offline;
 * \li analizzare ed eseguire calcoli in tempo reale sul flusso continuo dei
 * dati;
 * \li recuperare e convertire on-the-fly  i dati in diversi formati, (ad
 * esempio mini-SEED records);
 * \li redirezionare i dati in servers o sistemi di altro tipo, (ad esempio
 * SeedLink o Earthworm).
 * 
 * Al momento la libreria &egrave; in grado di trattare i dati di tipo time-series
 * e non quelli di tipo serial data, triggers e state-of-healt. Per
 * quest'ultimi si &egrave; rimandato lo sviluppo ad un futuro prossimo.
 * 
 *
 * \subsection installazione_sec Installazione
 * 
 * La libreria libnmxp e il tool nmxptool sono stati sviluppati utilizzando
 * i GNU Build Tools (automake e autoconf) tenendo conto degli aspetti di
 * compilazione trasverale (cross-compilation) per tutte le possibili
 * piattaforme di tipo POSIX/UNIX. Di seguito la tabella 1 mostra su quali
 * sistemi operativi e architetture si &egrave; eseguito il test di funzionamento,
 * la `X' determina che il test ha avuto esito positivo.
 * 
 * <table border="0">
 *
 * <tr><td>
 * <table border="1" align="center">
 *
 * <tr>
 * <td>&nbsp;</td>	<td align="center">Intel 32-bit</td>	<td align="center">Intel 64-bit</td>	<td align="center">SPARC 64bit</td>	<td align="center">PowerPC</td>
 * </tr>
 * 
 * <tr>
 * <td>Linux</td> <td align="center">X</td> <td align="center">X</td> <td align="center">&nbsp;</td> <td align="center">&nbsp;</td> 
 * </tr>
 * 
 * <tr>
 * <td>Solaris</td> <td align="center">X</td> <td align="center">&nbsp;</td> <td align="center">X</td> <td align="center">&nbsp;</td> 
 * </tr>
 * 
 * <tr>
 * <td>Mac OS X</td> <td align="center">X</td> <td align="center">&nbsp;</td> <td align="center">&nbsp;</td> <td align="center">X</td> 
 * </tr>
 * 
 * <tr>
 * <td>FreeBSD</td> <td align="center">X</td> <td align="center">&nbsp;</td> <td align="center">&nbsp;</td> <td align="center">&nbsp;</td> 
 * </tr>
 * 
 * </table>
 * </tr></td>
 *
 * <tr><td>
 * 	<b>Tabella 1. Sistemi operativi e architetture sui quali libnmxp e
 * nmxptool sono stati installati ed eseguiti con successo.</b>
 * </tr></td>
 * </table>
 * 
 *  
 * 
 * I sorgenti, la documentazione e gli scripts di installazione della
 * libreria e del programma vengono rilasciati in distribuzioni compresse,
 * con nome del tipo libnmxp-1.1.2.tar.gz. I requisiti per l'installazione
 * sono:
 * 
 * \li Piattaforma POSIX
 * \li Compilatore C GNU
 * \li Programma make GNU
 * 
 * Il modo pi&ugrave; semplice per compilare i sorgenti &egrave;:
 * \li `cd` nella directory che contiene lo script configure
 * \li Lanciare il comando ./configure
 * \li Se configure termina con esito positivo allora lanciare il comando make per la compilazione
 * \li Lanciare il comando make install per l'installazione
 * 
 * Quindi, a titolo di esempio, ecco la sequenza dei comandi da eseguire in
 * una shell per compilare libnmxp e nmxptool contenuti nella distribuzione
 * libnmxp-1.1.2.tar.gz:
 * 
 * <pre>
 * kyuzo:~ mtheo$ <b>tar xvfz libnmxp-1.1.2.tar.gz</b>
 * 
 * kyuzo:~ mtheo$ <b>cd libnmxp-1.1.2</b>
 * 
 * kyuzo:~/libnmxp-1.1.2 mtheo$ <b>./configure</b>
 * 
 * <i>
 * ...
 * 
 * config.status: creating Makefile
 * 
 * config.status: creating src/Makefile
 * 
 * config.status: creating config.h
 * 
 * config.status: executing depfiles commands
 * 
 * configure:
 * 
 *       After running make and make install you will be able
 * 
 *       to compile nmpxtool into the subdirectory tools/nmxptool.
 * 
 *       nmxptool is a tool that implements the following protocols:
 * 
 *                 * Nanometrics Data Access Protocol 1.0
 * 
 *                 * Nanometrics Private Data Stream  1.4
 *                 </i>
 * 
 * kyuzo:~/libnmxp-1.1.2 mtheo$ <b>make</b>
 * 
 * kyuzo:~/libnmxp-1.1.2 mtheo$ <b>su root</b>
 * 
 * kyuzo:~/libnmxp-1.1.2 root# <b>make install</b>
 * 
 * kyuzo:~/libnmxp-1.1.2 root# <b>exit</b>
 * 
 * kyuzo:~/libnmxp-1.1.2 mtheo$ <b>cd tools/nmxptool</b>
 * 
 * kyuzo:~/libnmxp-1.1.2/tools/nmxptool mtheo$ <b>./configure</b>
 * 
 * kyuzo:~/libnmxp-1.1.2/tools/nmxptool mtheo$ <b>make</b>
 * 
 * kyuzo:~/libnmxp-1.1.2/tools/nmxptool mtheo$ <b>su root</b>
 * 
 * kyuzo:~/libnmxp-1.1.2/tools/nmxptool root# <b>make install</b>
 * </pre>
 * 
 * Lo script configure automaticamente rileva e compila se presenti: la
 * libreria per  il salvataggio dei dati in mini-SEED,  i sorgenti con le
 * funzioni base di un plug-in SeedLink e i file oggetto (i file di tipo
 * .o) della libreria di Earthworm. Le compilazioni di queste tre
 * funzionalita a supporto di nmxptool possono essere inibite passando
 * rispettivamente al configure i seguenti tre parametri:
 * 
 *   <pre>
 *    --disable-libmseed      disable saving data in mini-SEED records
 *    --disable-ew            do not compile nmxptool as Earthworm module
 *    --disable-seedlink      do not compile nmxptool as Seedlink plug-in
 *   </pre>
 * 
 * Per configurare nmxptool come modulo Earthworm bisognera copiare i files
 * nmxptool.d e nmxptool.desc nella directory dei parametri di Earthworm e
 * poi modificarli secondo le proprie esigenze. La copia sara un comando
 * del  tipo:
 * 
 * <pre>
 * kyuzo:~/libnmxp-1.1.2/tools/nmxptool mtheo$ <b>cp earthworm/nmxptool.* ${EW_PARAMS}</b>
 * </pre>
 * 
 * Per poter configurare nmxptool come plug-in all'interno di SeisComP sara
 * sufficiente copiare la directory 135_nmxptool all'interno dei templates
 * di SeisComP. Supponendo la SeisComP Root uguale a /home/sysop/seiscomp,
 * ecco un esempio del comando da lanciare:
 * 
 * <pre>
 * kyuzo:~/libnmxp-1.1.2/tools/nmxptool mtheo$ <b>cp -r seiscomp_templates/135_nmxptool \
 *                                             /home/sysop/seiscomp/acquisition/templates/source/</b>
 * </pre>
 * 
 * 
 * Successivamente sara possibile configurare il plug-in per mezzo della
 * configurazione standard di SeisComP, ovvero lanciando il comando:
 * 
 * <pre>
 * kyuzo:~ mtheo$ <b>seiscomp config</b>
 * </pre>
 * 
 * \section documentazione_sec Documentazione
 * 
 * Le funzioni a cui prestare maggiore attenzione sono quelle che si
 * occupano della gestione del buffer dei pacchetti nelle connessioni di
 * tipo Raw Stream, cio&egrave; dei pacchetti compressi e con valore di
 * Short-term-complete uguale a -1. Per un canale sismico la funzione
 * nmxp_raw_stream_manage() si occupa di riordinare cronologicamente le
 * strutture NMXP_DATA_PROCESS che ad ogni chiamata le vengono passate,
 * successivamente di eseguire sulle stesse le n_func_pd funzioni i cui
 * puntatori sono contenuti nell'array p_func_pd. Nel caso in cui rilevi
 * una discontinuit&agrave; temporale del dato, la funzione accoda in un buffer la
 * struttura corrente inducendo cos&igrave; una latenza sul flusso dei dati per
 * quel canale. L'attesa dei pacchetti mancanti termina quando il tempo
 * massimo di latenza tollerabile, impostato al momento
 * dell'inizializzazione per mezzo della funzione nmxp_raw_stream_init(),
 * viene superato. In quest'ultimo caso la funzione forzera l'esecuzione
 * delle funzioni sulla prima struttura disponibile causando quindi un gap
 * sul flusso dei dati.
 * 
 *
 * \subsection uso_api_sec Uso delle APIs per sviluppare una nuova applicazione
 * 
 * Per sviluppare una propria applicazione in C che faccia uso della
 * libreria libnmxp vengono di seguito illustrati i sorgenti 1 e 2 che
 * possono essere utilizzati come base per l'implementazione dei protocolli
 * Data Access Protocol 1.0 e Private Data Stream 1.4. Su tali strutture di
 * codice C &egrave; basato anche nmxptool descritto successivamente.
 * 
 * E' importante notare come risulti relativamente semplice sviluppare una
 * propria applicazione anche nel caso in cui si vogliano stabilire
 * connessioni di tipo Raw Stream. Infatti lo sviluppatore non dovra far
 * altro che utilizzare la struttura base del sorgente 2, eseguire le
 * opportune personalizzazioni, e dichiarare una funzione con prototipo
 * 
 * \code
 * int ( *process_data_function ) ( NMXP_DATA_PROCESS *)
 * \endcode
 * 
 * il cui puntatore dovra poi essere aggiunto nell'array da passare come
 * parametro alla funzione nmxp_raw_stream_manage().
 * 
 * Prima di poter richiamare la funzione nmxp_raw_stream_manage() bisogna
 * inizializzare per ogni canale, tramite la funzione
 * nmxp_raw_stream_init(), una struttura dati di tipo NMXP_RAW_STREAM_DATA
 * e il valore della massima latenza tollerabile.
 * 
 * Al termine del programma, o comunque al termine della connessione, sara
 * necessario liberare la memoria allocata dalla struttura
 * NMXP_RAW_STREAM_DATA per mezzo della funzione nmxp_raw_stream_free().
 * Opzionalmente, prima di questa funzione pu&ograve; essere richiamata
 * nmxp_raw_manage_stream_flush() che esegue le funzioni sui pacchetti
 * rimanenti indipendentemente dalla continuit&agrave; del dato.
 * 
 * \todo
 * - Sorgente 1. Struttura base in C che implementa D.A.P. versione 1.0 utilizzando le APIs di libnmxp.
 * - Sorgente 2. Struttura base in C che implementa P.D.S. versione 1.4 utilizzando le APIs di libnmxp.
 * 
 * 
 * \section nmxptool_sec Il programma nmxptool
 * 
 * Al fine di capire cosa nxmptool permette di fare, lanciamo inizialmente
 * il comando che stampa a video terminale l'help delle opzioni del
 * comando:
 * 
 * <pre>
kyuzo:~ mtheo$ nmxptool -h

nmxptool 1.1.5, Nanometrics tool based on libnmxp-1.1.5
        (Data Access Protocol 1.0, Private Data Stream 1.4)
         Support for: libmseed YES, SeedLink YES, Earthworm YES.

Usage: nmxptool -H hostname --listchannels [...]
             Receive list of available channels on the host

       nmxptool -H hostname -C channellist -s DATE -e DATE [...]
       nmxptool -H hostname -C channellist -s DATE -t SECs [...]
             Receive data from hostname by DAP

       nmxptool -H hostname -C channellist [...]
             Receive data from hostname by PDS

       nmxptool nmxptool.d
             Run as earthworm module receiving data by PDS

Arguments:
  -H, --hostname=HOST     Nanometrics hostname.
  -C, --channels=LIST     Channel list NET.STA.CHAN (NET. is optional)
                             N1.STA1.HH?,N2.STA2.??Z,STA3.?H?,...
                          NET is used only for output!

Other arguments:
  -P, --portpds=PORT      NaqsServer port number (default 28000).
  -D, --portdap=PORT      DataServer port number (default 28002).
  -N, --network=NET       Default Network code for stations without value. (default 'XX').
  -L, --location=LOC      Location code for writing file.
  -v, --verbose           Be verbose.
  -g, --logdata           Print info about data.
  -m, --writeseed         Pack received data in Mini-SEED records and write to a file.
  -w, --writefile         Dump received data to a file.
  -k, --slink=plug_name   Send received data to SeedLink like as plug-in.
                          plug_name is set by SeisComP daemon.
                          THIS OPTION MUST BE THE LAST WITHOUT plug_name IN seedlink.ini!
  -V, --version           Print tool version.
  -h, --help              Print this help.

DAP Arguments:
  -s, --start_time=DATE   Start time in date format.
  -e, --end_time=DATE     End time in date format.
                          DATE can be in formats:
                              \<date\>,\<time\> | \<date\>
                          where:
                              \<date\> = yyyy/mm/dd | yyy.jjj
                              \<time\> = hh:mm:ss | hh:mm
  -t, --interval=SECs     Time interval from start_time.
  -d, --delay=SECs        Receive continuosly data with delay [60..86400].
  -u, --username=USER     DataServer username.
  -p, --password=PASS     DataServer password.
  -l, --listchannels      Output list of channel available on DataServer.
  -i, --channelinfo       Output list of channel available on DataServer and channelinfo.

PDS arguments:
  -S, --stc=SECs          Short-term-completion (default -1).
                          -1 is for Raw Stream, no short-term completion.
                           0 chronological order without waiting for missing data.
                          [0..300] wait a period for the gap to be filled by retransmitted packets.
                          Raw Stream is usable only with --rate=-1.
  -R, --rate=Hz           Receive data with specified sample rate (default -1).
                          -1 is for original sample rate and compressed data.
                           0 is for original sample rate and decompressed data.
                          >0 is for specified sample rate and decompressed data.
  -b, --buffered          Request also recent packets into the past.
  -M, --maxlatency=SECs   Max tolerable latency (default 600) [60..600].
  -T, --timeoutrecv=SECs  Time-out receiving packets (default 0. No time-out) [10..300].
                          -T is useful for retrieving Data On Demand.
                          -M, -T are usable only with Raw Stream --stc=-1.

Matteo Quintiliani - Istituto Nazionale di Geofisica e Vulcanologia - Italy
Mail bug reports and suggestions to <quintiliani@ingv.it>.
 * </pre>
 * 
 * 
 * 
 * Da tale output deduciamo che un parametro sempre necessario &egrave; il nome o
 * l'IP del server al quale richiedere i dati. Il programma, in funzione
 * dei parametri passati, determina automaticamente se effettuare una
 * connessione al NaqsServer (porta 28000) oppure al DataServer (porta
 * 28002). Se le porte dei servers non sono quelle di default &egrave; necessario
 * utilizzare le opzioni -P e -D. Un primo utilizzo di nmxptool per esempio
 * potrebbe essere quello di impiegarlo per reperire la lista dei canali
 * disponibili sul server e dei tempo di inizio e fine dei dati per ogni
 * canale. Ci&ograve; si ottiene per mezzo del comando:
 * 
 * <pre>
 * kyuzo:~ mtheo$ nmxptool -H hostname -l
 * </pre>
 * 
 * Una parte di un possibile output:
 * 
 * <pre>
 * ...
 * 1255538946 USI.HHE.        (2007.233,10:39:21.0000  - 2007.243,09:59:44.0000)
 * 1255538945 USI.HHN.        (2007.233,16:20:53.0000  - 2007.243,09:59:45.0000)
 * 1255538944 USI.HHZ.        (2007.233,22:26:08.0000  - 2007.243,09:59:31.0000)
 * 1238565122 VAGA.HHE.       (2007.225,07:10:14.0000  - 2007.243,09:59:19.0000)
 * 1238565121 VAGA.HHN.       (2007.225,08:35:24.0000  - 2007.243,09:59:29.0000)
 * 1238565120 VAGA.HHZ.       (2007.225,00:03:14.0000  - 2007.243,09:59:29.0000)
 * ...
 * </pre>
 * 
 * 
 * 
 * Per ogni canale disponibile viene visualizzato:
 * 
 * \li l'indice numerico Nanometrics del canale, denominato key channel
 * \li il nome del canale nella forma Station.Channel.Network
 * \li data e ora di inizio dei dati disponibili
 * \li data e ora di fine dei dati disponibili
 * 
 * Successivamente potremmo richiedere al DataServer i dati appartenenti ad
 * un certo intervallo di tempo e di un insieme di canali, il comando
 * allora dovra contenere le opzioni -s, -e, -C,  quindi ad esempio:
 * 
 * <pre>
 * kyuzo:~ mtheo$ nmxptool -H hostname -s 2007.242,00:00 -e 2007/08/30,00:00:05 \
 *                                                       -C IV.USI.???,VAGA.HHZ -g
 * </pre>
 *
 * In alternativa al posto dell'opzione -e si pu&ograve; utilizzare l'opzione -t
 * che specifica la quantit&agrave; in secondi di dati da richiedere.
 * 
 * Osserviamo che la data pu&ograve; essere scritta seguendo tali regole:
 * 
 * DATA,ORA oppure solamente DATA, dove:
 * 
 * DATA pu&ograve; essere espressa nei seguenti formati:
 * 
 *      - aaaa/mm/gg
 *      - aaa.jjj                 (jjj &egrave; il giorno giuliano dell'anno)
 * 
 * ORA pu&ograve; essere espressa nei seguenti formati:
 * 
 *     - hh:mm:ss
 *     - hh:mm
 * 
 * Se si specifica solo DATA, ORA verra automaticamente impostata a 00:00
 * 
 * 
 * Notiamo inoltre che la lista dei canali pu&ograve; contenere il carattere
 * speciale ? che ha il significato di ``qualsiasi carattere''. Alla riga
 * di comando abbiamo aggiunto anche l'opzione -g che visualizza
 * informazioni su ogni pacchetto ricevuto. Ecco un output possibile:
 * 
 * <pre>
 * IV.USI.HHE 100Hz (2007.242,00:00:00.0000 - 2007.242,00:00:00.8699) lat 130115.1s [1, 48353370] (0)   87pts (-1128, -1128, 1742, 3226, 1) 276
 * IV.USI.HHE 100Hz (2007.242,00:00:00.8699 - 2007.242,00:00:01.9899) lat 130114.0s [1, 48353371] (0)  112pts (3226, 3226, 2423, 2688, 1) 276
 * IV.USI.HHE 100Hz (2007.242,00:00:01.9900 - 2007.242,00:00:03.1099) lat 130112.9s [1, 48353372] (0)  112pts (2688, 2688, -548, -686, 1) 276
 * IV.USI.HHE 100Hz (2007.242,00:00:03.1099 - 2007.242,00:00:04.2500) lat 130111.8s [1, 48353373] (0)  114pts (-686, -686, -857, -74, 1) 276
 * IV.USI.HHE 100Hz (2007.242,00:00:04.2500 - 2007.242,00:00:05.0000) lat 130111.0s [1, 48353374] (0)   75pts (-74, -74, 1290, 1338, 1) 276
 * IV.USI.HHN 100Hz (2007.242,00:00:00.0000 - 2007.242,00:00:00.2500) lat 130116.8s [1, 49688091] (0)   25pts (301, 301, 11, -143, 1) 276
 * IV.USI.HHN 100Hz (2007.242,00:00:00.2500 - 2007.242,00:00:01.3699) lat 130115.6s [1, 49688092] (0)  112pts (-143, -143, 926, 1534, 1) 276
 * IV.USI.HHN 100Hz (2007.242,00:00:01.3699 - 2007.242,00:00:02.5099) lat 130114.5s [1, 49688093] (0)  114pts (1534, 1534, -220, -17, 1) 276
 * IV.USI.HHN 100Hz (2007.242,00:00:02.5099 - 2007.242,00:00:03.6299) lat 130113.4s [1, 49688094] (0)  112pts (-17, -17, -866, -837, 1) 276
 * IV.USI.HHN 100Hz (2007.242,00:00:03.6300 - 2007.242,00:00:04.7900) lat 130112.2s [1, 49688095] (0)  116pts (-837, -837, -716, -527, 1) 276
 * IV.USI.HHN 100Hz (2007.242,00:00:04.7899 - 2007.242,00:00:05.0000) lat 130112.0s [1, 49688096] (0)   21pts (-527, -527, 999, 790, 1) 276
 * IV.USI.HHZ 100Hz (2007.242,00:00:00.0000 - 2007.242,00:00:00.4400) lat 130116.6s [1, 50549101] (0)   44pts (-5470, -5470, -4031, -4326, 1) 276
 * IV.USI.HHZ 100Hz (2007.242,00:00:00.4400 - 2007.242,00:00:01.5599) lat 130115.4s [1, 50549102] (0)  112pts (-4326, -4326, -6154, -6408, 1) 276
 * IV.USI.HHZ 100Hz (2007.242,00:00:01.5599 - 2007.242,00:00:02.6799) lat 130114.3s [1, 50549103] (0)  112pts (-6408, -6408, -5355, -5326, 1) 276
 * IV.USI.HHZ 100Hz (2007.242,00:00:02.6800 - 2007.242,00:00:03.7999) lat 130113.2s [1, 50549104] (0)  112pts (-5326, -5326, -4203, -4963, 1) 276
 * IV.USI.HHZ 100Hz (2007.242,00:00:03.7999 - 2007.242,00:00:04.9199) lat 130112.1s [1, 50549105] (0)  112pts (-4963, -4963, -4980, -5066, 1) 276
 * IV.USI.HHZ 100Hz (2007.242,00:00:04.9200 - 2007.242,00:00:05.0000) lat 130112.0s [1, 50549106] (0)    8pts (-5066, -5066, -4823, -4804, 1) 276
 * XX.VAGA.HHZ 100Hz (2007.242,00:00:00.0000 - 2007.242,00:00:00.2999) lat 130116.7s [1, 7848381] (0)   30pts (-10567, -10567, -10553, -10550, 1) 276
 * XX.VAGA.HHZ 100Hz (2007.242,00:00:00.2999 - 2007.242,00:00:02.5399) lat 130114.5s [1, 7848382] (0)  224pts (-10550, -10550, -10456, -10458, 1) 276
 * XX.VAGA.HHZ 100Hz (2007.242,00:00:02.5399 - 2007.242,00:00:04.7799) lat 130112.2s [1, 7848383] (0)  224pts (-10458, -10458, -10363, -10362, 1) 276
 * XX.VAGA.HHZ 100Hz (2007.242,00:00:04.7799 - 2007.242,00:00:05.0000) lat 130112.0s [1, 7848384] (0)   22pts (-10362, -10362, -10331, -10337, 1) 276
 * </pre>
 * 
 * 
 * Per ogni pacchetto ricevuto viene visualizzato:
 * 
 * \li il nome del canale nella forma Network.Station.Channel
 * \li la frequenza di campionamento
 * \li i tempi del primo e dell'ultimo campione
 * \li la latenza in secondi rispetto all'ora del client
 * \li il tipo del pacchetto Nanometrics e il suo numero di sequenza
 * \li il valore del numero di sequenza del pi&ugrave; vecchio pacchetto disponibile
 * \li il numero di campioni presenti nel pacchetto
 * \li il valore x0 contenuto nell'intestazione (header) del pacchetto Nanometrics
 * \li il primo e l'ultimo valore della serie di campioni
 * \li il valore xn, ovvero il valore calcolato che dovra avere x0 nel pacchetto successivo
 * \li il flag che indica se x0 e xn sono significativi (0 significativo, -1 non significativo)
 * \li la lunghezza in bytes del pacchetto Nanometrics ricevuto
 * 
 * Nell'esempio precedente, non essendo stata definita la rete (network),
 * per default il programma l'ha impostata a `XX'. Nel caso avessimo voluto
 * salvare i dati in formato mini-SEED sarebbe stato sufficiente aggiungere
 * l'opzione -m e il programma avrebbe generato un file per ogni canale.
 * Inoltre, se il DataServer avesse richiesto l'autenticazione si sarebbero
 * dovute utilizzare le opzioni per la definizione del nome utente e della
 * password, ovvero -u e -p.
 * 
 * Per avere un flusso di dati continuo ma in differita con uno specifico
 * tempo stabilito &egrave; possibile utilizzare l'opzione -d. In questo modo si
 * ricevono quindi dati in flusso continuo dal DataServer tenendo fissa la
 * latenza al valore impostato. Ad esempio, per 1 ora (3600 secondi) di
 * differita, un possibile comando sara:
 * 
 * <pre>
 * kyuzo:~ mtheo$ nmxptool -H hostname -d 3600 -C USI.???,VAGA.HHZ -g
 * </pre>
 * 
 * Per ricevere dati in tempo reale, ovvero da un NaqsServer, i
 * sufficiente, in generale, non definire l'intervallo temporale. Quindi un
 * comando del tipo:
 * 
 * <pre>
 * kyuzo:~ mtheo$ nmxptool -H hostname -C USI.??? -g -R 100
 * </pre>
 * 
 * restituirebbe un output simile a questo di seguito:
 * 
 * <pre>
 * IV.USI.HHN 100Hz (2007.243,12:22:48.0000 - 2007.243,12:22:49.0000) lat 9.0s [4, -1] (-1)  100pts (-1, 2080, 2488, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:48.0000 - 2007.243,12:22:49.0000) lat 9.0s [4, -1] (-1)  100pts (-1, 703, 2789, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:49.0000 - 2007.243,12:22:50.0000) lat 8.0s [4, -1] (-1)  100pts (-1, 2947, -1268, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:49.0000 - 2007.243,12:22:50.0000) lat 8.0s [4, -1] (-1)  100pts (-1, 1924, 204, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:49.0000 - 2007.243,12:22:50.0000) lat 8.0s [4, -1] (-1)  100pts (-1, 2490, -1004, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:50.0000 - 2007.243,12:22:51.0000) lat 7.0s [4, -1] (-1)  100pts (-1, -931, 1006, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:50.0000 - 2007.243,12:22:51.0000) lat 7.0s [4, -1] (-1)  100pts (-1, -1131, 1239, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:50.0000 - 2007.243,12:22:51.0000) lat 7.0s [4, -1] (-1)  100pts (-1, -103, -588, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:51.0000 - 2007.243,12:22:52.0000) lat 6.0s [4, -1] (-1)  100pts (-1, 951, 3495, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:51.0000 - 2007.243,12:22:52.0000) lat 6.0s [4, -1] (-1)  100pts (-1, 1318, 790, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:51.0000 - 2007.243,12:22:52.0000) lat 6.0s [4, -1] (-1)  100pts (-1, -467, 93, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:52.0000 - 2007.243,12:22:53.0000) lat 5.0s [4, -1] (-1)  100pts (-1, 365, 956, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:52.0000 - 2007.243,12:22:53.0000) lat 5.0s [4, -1] (-1)  100pts (-1, 3356, 2437, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:52.0000 - 2007.243,12:22:53.0000) lat 5.0s [4, -1] (-1)  100pts (-1, 1034, 1527, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:53.0000 - 2007.243,12:22:54.0000) lat 4.0s [4, -1] (-1)  100pts (-1, 951, 16, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:53.0000 - 2007.243,12:22:54.0000) lat 4.0s [4, -1] (-1)  100pts (-1, 2559, -319, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:53.0000 - 2007.243,12:22:54.0000) lat 4.0s [4, -1] (-1)  100pts (-1, 1472, 675, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:54.0000 - 2007.243,12:22:55.0000) lat 3.0s [4, -1] (-1)  100pts (-1, 255, -351, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:54.0000 - 2007.243,12:22:55.0000) lat 3.0s [4, -1] (-1)  100pts (-1, -668, 1457, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:54.0000 - 2007.243,12:22:55.0000) lat 3.0s [4, -1] (-1)  100pts (-1, 1101, 1541, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:55.0000 - 2007.243,12:22:56.0000) lat 2.0s [4, -1] (-1)  100pts (-1, -540, 1162, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:55.0000 - 2007.243,12:22:56.0000) lat 2.0s [4, -1] (-1)  100pts (-1, 1593, -488, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:55.0000 - 2007.243,12:22:56.0000) lat 2.0s [4, -1] (-1)  100pts (-1, 1608, 1355, -1, 0) 420
 * IV.USI.HHE 100Hz (2007.243,12:22:56.0000 - 2007.243,12:22:57.0000) lat 1.0s [4, -1] (-1)  100pts (-1, 1324, -1674, -1, 0) 420
 * IV.USI.HHN 100Hz (2007.243,12:22:56.0000 - 2007.243,12:22:57.0000) lat 2.0s [4, -1] (-1)  100pts (-1, -371, 2315, -1, 0) 420
 * IV.USI.HHZ 100Hz (2007.243,12:22:56.0000 - 2007.243,12:22:57.0000) lat 2.0s [4, -1] (-1)  100pts (-1, 1279, 967, -1, 0) 420
 * </pre>
 * 
 * 
 * L'opzione -R &egrave; stata utilizzata per dichiarare che i pacchetti da
 * ricevere sarebbero stati scompattati dal server con una frequenza di
 * 100Hz. Notiamo infatti che il pacchetto &egrave; di tipo 4, ovvero decompresso,
 * con una capacit&agrave; fissa di un secondo e che x0 e xn non sono
 * significativi. Invece per i pacchetti compressi (pacchetto di tipo 1)
 * avremmo anche pouto specificare, tramite l'opzione -S, un valore fra 1 e
 * 300 secondi dello Short-term-complete, oppure 0 per nessun
 * Short-term-complete,  oppure uguale -1 per ricevere i pacchetti in
 * modalita Raw Stream. 
 * 
 * Quest'ultimo caso rappresenta una delle funzionalita pi&ugrave; importanti di
 * nmxptool poich&eacute; consente di ricevere i pacchetti in modo continuo in
 * tempo reale, in ordine cronologico, con minima latenza e minimo numero
 * di gaps.  Il programma  &egrave; in grado di gestire il buffering dei pacchetti
 * trasmessi e ritrasmessi, il loro riordinamento e l'esecuzione delle
 * operazione selezionate tramite le opzioni. Un'opzione collegata a questa
 * gestione &egrave; -M che serve a specificare la massima latenza tollerabile
 * nell'attesa di un pacchetto mancante. Di conseguenza da tale opzione
 * dipende la grandezza del buffer.
 *
 * Alternativamente, o in aggiunta all'opzione -M si pu&ograve; utilizzare l'opzione -T
 * che specifica per ogni canale il tempo massimo tollerabile fra la ricezione
 * di un pacchetto e il successivo.
 * 
 * Quando si interagisce con il NaqsServer si pu&ograve; anche utilizzare
 * l'opzione -b, la quale permette di ricevere anche alcuni dati, in
 * quantita discrezionale del server, che precedono quelli dell'istante
 * attuale di richiesta.
 * 
 * 
 * 
 * \image html doc/images/nmxp_data_flow.jpg "Figura 2. Localizzazione degli eventi e archiviazione dei dati sismici in tempo reale e completamento in differita. Le tre attivita si basano con modalita diverse su nmxptool. Nel primo e nel secondo caso, nmxptool si connette al NaqsServer in modalita Raw Stream e viene eseguito rispettivamente come modulo del sistema Earthworm e come plug-in SeedLink. Nel terzo i dati mancanti vengono richiesti da nmxptool al DataServer e ricongiunti alla struttura di archiviazione SDS di SeisComP."
 * 
 * 
 * \subsection modulo_eathworm_sec Modulo Earthworm
 * 
 * nmxptool pu&ograve; essere eseguito come modulo del sistema Earthworm.
 * Generalmente il tipo di connessione eseguita &egrave; di tipo Raw Stream e i
 * parametri, invece di essere passati tramite linea di comando, vengono
 * letti da un file di configurazione tipo .d, rispettando cos&igrave; lo standard
 * dei moduli Earthworm. All'interno della distribuzione sono disponibili i
 * due files nmxptool.d e nmxptool.desc, i quali possono essere usati come
 * base per la configurazione di nmxptool all'interno del sistema
 * Earthworm. E' comunque in corso la richiesta per inserire nmxptool nelle
 * distribuzioni ufficiali di Earthworm.
 * 
 *
 * \subsection plugin_seedlink_sec Plug-in SeedLink
 * 
 * Con qualsiasi configurazione di opzioni descritte precedentemente,
 * nmxptool pu&ograve; essere lanciato come un plug-in per SeedLink per mezzo
 * dell'utilizzo dell'opzione -k. Questa opzione deve essere
 * necessariamente dichiarata per ultima. All'interno della distribuzione
 * sono inoltre disponibili i templates SeedLink necessari alla
 * configurazione del plug-in tramite  il comando ``seiscomp config''. E'
 * comunque in corso la richiesta per inserire nmxptool fra i plug-ins
 * delle distribuzioni ufficiali di SeisComP.
 * 
 *
 * \subsection completezza_dato_nanometrics_sec Completezza del dato Nanometrics
 * 
 * La figura 2 illustra come nmxptool viene utilizzato all'interno
 * dell'INGV per far fluire i dati sismici delle stazioni che trasmettono
 * tramite i protocolli Nanometrics nei sistemi Earthworm e SeisComP. Le
 * forme d'onda vengono ricevute in tempo reale in modalita Raw Stream al
 * fine di minimizzare la latenza e il numero di gaps. nmxptool viene
 * configurato e lanciato all'interno del sistema Earthworm per consentire
 * il calcolo delle localizzazioni degli eventi sismici ed inoltre viene
 * configurato e lanciato all'interno del sistema SeisComp come plug-in
 * SeedLink per l'archiviazione dei dati. La latenza indotta dal programma
 * &egrave; determinata solo nel caso in cui si rimanga in attesa di uno o pi&ugrave;
 * pacchetti mancanti. Tale attesa termina nel momento in cui il buffer
 * risulti completamente pieno comportando quindi una perdita di dati
 * (gap). Il valore impostato per la massima latenza tollerabile sara, in
 * generale, minore per localizzare un evento (ad esempio 30-60 sec.)
 * rispetto a quello impostato per l'archiviazione (ad esempio 300-600
 * sec.).
 * 
 * Al fine di garantire completezza dei dati archiviati &egrave; stata sviluppata
 * una procedura dal nome nmdc, ovvero ``Nanometrics Data Completeness'',
 * che basandosi sulla versatilita di nmxptool recupera i dati mancanti
 * dopo qualche ora o il giorno successivo. In questo caso i gaps
 * risultanti non potranno pi&ugrave; essere colmati poich&eacute; i dati richiesti non
 * risultano pi&ugrave; essere definitivamente presenti sul lato dei servers
 * Nanometrics.
 * 
 *
 * \section conclusioni_sec Conclusioni
 * 
 * Lo sviluppo e i test eseguiti in questi ultimi due mesi, hanno permesso
 * di realizzare una libreria nel suo complesso stabile ed efficiente.
 * Considerando congiuntamente libnmxp e nmxptool, anche il numero di
 * funzionalita implementate risulta essere molto soddisfacente. La pi&ugrave;
 * importante fra tutte &egrave; sicuramente la gestione delle connessioni di tipo
 * Raw Stream, che riesce a garantire una bassa latenza e nel contempo un
 * numero minino di gaps. Grazie a questa caratteristica nmxptool apporta,
 * per quanto concerne l'acquisizione da servers Nanometrics, un
 * fondamentale contributo alle comunita degli utilizzatori dei sistemi
 * SeisComp e Earthworm. Infatti sia naqs_plugin, l'attuale plug-in per
 * SeedLink, che naqs2ew, l'attuale modulo per Earthworm, non essendo in
 * grado di gestire connessioni di tipo Raw Stream, non possono garantire
 * minimamente la continuit&agrave; del dato al verificarsi di una ritrasmissione
 * dal lato Nanometrics Server - Stazione Sismica (fig. 1).
 * 
 * Le tabelle 2 e 3 mostrano i reports sintetici dei dati archiviati per
 * alcuni canali in test il 22 e il 23 settembre 2007. Giornalmente, per
 * ogni canale viene visualizzato:
 * 
 * \li Totale di pacchetti ritrasmessi: per una trasmissione di tipo
 * short-term-complete i dati contenuti in questi pacchetti sarebbero stati
 * persi e avrebbero causato dei gaps.
 * \li Massima latenza registrata: la massima latenza registrata durante il
 * giorno e indotta dall'attesa dei pacchetti mancanti. Per tale test la
 * latenza massima tollerabile &egrave; stata impostata a 600 secondi.
 * \li Numero di gaps ottenuti in tempo reale tramite l'acquisizione dei dati
 * per mezzo di nmxptool usato come plug-in SeedLink. Possiamo notare come
 * il numero di gaps ottenuti in tempo reale dipenda fortemente dall'attesa
 * dei pacchetti ritrasmessi.
 * \li Numero di gaps definitivi: ottenuti recuperando i dati il giorno dopo dal
 * DataServer per mezzo di nmxptool usato all'interno della procedura nmdc,
 * ``Nanometrics Data Completeness''.
 * \li Percentuale dei pacchetti persi in tempo reale: il valore &egrave; il risultato
 * della seguente espressione: [ (Gaps Tempo Reale - Gaps Definitivi) /
 * Pacchetti Ritrasmessi ] * 100. Questo valore pu&ograve; essere interpretato
 * come espressione della bonta nella scelta del valore di massima latenza
 * tollerabile per quel dato canale. Su questo valore e secondariamente
 * sulla latenza massima sono state ordinate le due tabelle.
 * 
 * Normalmente il dato definitivo dovrebbe essere continuo e completo,
 * quindi la presenza di un numero rilevante di gaps dovrebbe evidenziare
 * in qualche modo un'anomalia o un problema nel sistema di acquisizione.
 * E' questo infatti il caso verificatosi per la stazione di MONC durante
 * il test. Esaminando i log di nmxptool si constata che tali gaps sono
 * fittizi poich&eacute; dovuti a errati valori temporali all'interno dei
 * pacchetti e quindi probabilmente determinati da un mal funzionamento del
 * GPS.
 * 
 * Al momento nmxptool viene utilizzato come plug-in SeedLink anche nei
 * seguenti istituti di ricerca ai quali va anche un riconoscimento per la
 * loro collaborazione in fase di test e debugging del software:
 * 
 * \li <i>National Data Center</i>, Israele. (Guy Tikochinsky)
 * \li <i>Institute of Geodynamics</i>, National Observatory of Athens, Grecia. (Nicos
 * Melis)
 * 
 * Possiamo quindi sintetizzare i risultati ottenuti rilevando che
 * l'utilizzo di nmxptool con connessioni in tempo reale al NaqsServer
 * (online) di tipo Raw Stream, garantisce un ottimo compromesso fra
 * continuit&agrave; del dato e latenza indotta, mentre il suo utilizzo con
 * connessioni in differita al DataServer (offline) garantisce pienamente
 * la completezza del dato (fig. 2).
 * 
 * E' evidente inoltre che la versalita di nmxptool e il numero di
 * funzionalita offerte, sono un valido supporto a procedure che
 * necessitano di dati a richiesta, come ad esempio il calcolo della
 * magnitudo o del momento tensore dopo un evento sismico.
 * 
 * Per il futuro l'autore intende manutenere libnmxp e nmxptool ed ampliare
 * le funzionalita della libreria anche per quanto riguarda la gestione dei
 * dati di tipo serial data, triggers, events e state-of-health.
 * 
 * 
 * \todo
 * - Tabella 2. Report sintetico relativo ai dati archiviati dei canali in test il 22 settembre 2007 tramite nmxptool e seedllink.
 * - Tabella 3. Report sintetico relativo ai dati archiviati dei canali in test il 23 settembre 2007 tramite nmxptool e seedllink.
 * 
 * 
 * 
 * \section ringraziamenti_sec Ringraziamenti
 * 
 * Un particolare ringraziamento va al Dott. Salvatore Mazza per la fiducia
 * che sempre mi riserva. Sono inoltre riconoscente al Dott. Marco Olivieri
 * per il suo supporto nel controllo di qualita dei dati prodotti dalle mie
 * applicazioni.
 * 
 *
 * \section bibliografia_sec Bibliografia e riferimenti web
 * 
 * Nanometrics, Inc., (1989-2002), Libra Satellite Seismograph System - Training Course Notes.
 * 
 * SeisComP, The Seismological Communication Processor
 * http://www.gfz-potsdam.de/geofon/seiscomp/
 * 
 * Earthworm, Seismic network data acquisition and processing system
 * http://www.isti2.com/ew/
 * 
 * libmseed, 2.1.4, The Mini-SEED library
 * http://www.iris.edu/manuals/
 * 
 * Doxygen, Source code documentation generator tool
 * 			http://www.stack.nl/~dimitri/doxygen/
 * 
 * GNU General Public License
 * 			http://www.gnu.org/copyleft/gpl.html
 *
 */



#ifndef NMXP_H
#define NMXP_H 1

#include "nmxp_base.h"
#include "nmxp_crc32.h"
#include "nmxp_memory.h"

#define NMXP_MAX_MSCHAN_MSEC		15000

/*! \brief Flag for buffered packets */
typedef enum {
    NMXP_BUFFER_NO			= 0,
    NMXP_BUFFER_YES			= 1
} NMXP_BUFFER_FLAG;


#define NMXP_MAX_SIZE_USERNAME 12

/*! \brief Body of ConnectRequest message*/
typedef struct {
    char username[NMXP_MAX_SIZE_USERNAME];
    int32_t version;
    int32_t connection_time;
    int32_t crc32;
} NMXP_CONNECT_REQUEST; 

/*! \brief Body of DataRequest message*/
typedef struct {
    int32_t chan_key;
    int32_t start_time;
    int32_t end_time;
} NMXP_DATA_REQUEST;

#define NMXP_MAX_FUNC_PD 10
#define TIME_TOLLERANCE 0.001

typedef struct {
    int32_t last_seq_no_sent;
    double last_sample_time;
    double last_latency;
    int32_t max_pdlist_items;
    double max_tolerable_latency;
    int timeoutrecv;
    int32_t n_pdlist;
    NMXP_DATA_PROCESS **pdlist; /* Array for pd queue */
} NMXP_RAW_STREAM_DATA;


/*! \brief Sends the message "Connect" on a socket
 *
 * \param isock A descriptor referencing the socket.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_sendConnect(int isock);


/*! \brief Sends the message "TerminateSubscription" on a socket
 *
 * \param isock A descriptor referencing the socket.
 * \param reason Reason for the shutdown.
 * \param message String message. It could be NULL.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_sendTerminateSubscription(int isock, NMXP_SHUTDOWN_REASON reason, char *message);


/*! \brief Receive message "NMXP_CHAN_LIST" from a socket
 *
 * \param isock A descriptor referencing the socket.
 * \param[out] pchannelList List of channels. It will need to be freed!
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_receiveChannelList(int isock, NMXP_CHAN_LIST **pchannelList);


/*! \brief Sends the message "AddTimeSeriesChannels" on a socket
 *
 * \param isock A descriptor referencing the socket.
 * \param channelList List of channel.
 * \param shortTermCompletion Short-term-completion time = s, 1<= s <= 300 seconds.
 * \param out_format Output format.
 * 	-1 Compressed packets.
 * 	0 Uncompressed packets.
 * 	0 < out_format, requested output sample rate.
 * \param buffer_flag Server will send or not buffered packets.
 * \param n_channel number of channels to add any time
 * \param n_usec frequency to add remaining channels (microseconds)
 * \param flag_restart reset index for requesting channels. In general, first time 1, then 0.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_sendAddTimeSeriesChannel(int isock, NMXP_CHAN_LIST_NET *channelList, int32_t shortTermCompletion, int32_t out_format, NMXP_BUFFER_FLAG buffer_flag, int n_channel, int n_usec, int flag_restart);


/*! \brief Receive Compressed or Decompressed Data message from a socket and launch func_processData() on the extracted data
 *
 * \param isock A descriptor referencing the socket.
 * \param channelList Channel list.
 * \param network_code Network code. It can be NULL.
 * \param location_code Location code. It can be NULL.
 * \param timeoutsec Time-out in seconds
 * \param[out] recv_errno errno value after recv()
 *
 * \retval Pointer to the structure NMXP_DATA_PROCESS on success
 * \retval NULL on error
 * 
 */
NMXP_DATA_PROCESS *nmxp_receiveData(int isock, NMXP_CHAN_LIST_NET *channelList, const char *network_code, const char *location_code, int timeoutsec, int *recv_errno );


/*! \brief Sends the message "ConnectRequest" on a socket
 *
 * \param isock A descriptor referencing the socket.
 * \param naqs_username User name (maximum 11 characters), zero terminated.
 * \param naqs_password Password.
 * \param connection_time Time that the connection was opened.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_sendConnectRequest(int isock, char *naqs_username, char *naqs_password, int32_t connection_time);


/*! \brief Read connection time from a socket
 *
 * \param isock A descriptor referencing the socket.
 * \param[out] connection_time Time in epoch.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_readConnectionTime(int isock, int32_t *connection_time);


/*! \brief Wait the message "Ready" from a socket
 *
 * \param isock A descriptor referencing the socket.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_waitReady(int isock);


/*! \brief Sends the message "DataRequest" on a socket
 *
 * \param isock A descriptor referencing the socket.
 * \param key Channel key for which data are requested.
 * \param start_time Start time of the interval for which data are requested. Epoch time.
 * \param end_time End time of the interval for which data are requested. Epoch time.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_sendDataRequest(int isock, int32_t key, int32_t start_time, int32_t end_time);


/*! \brief Sends the message "RequestPending" on a socket
 *
 * \param isock A descriptor referencing the socket.
 *
 * \retval SOCKET_OK on success
 * \retval SOCKET_ERROR on error
 * 
 */
int nmxp_sendRequestPending(int isock);

/*! \brief Get the list of available channels from a server 
 *
 * \param hostname host name
 * \param portnum port number
 * \param datatype Type of data contained in the channel.
 * \param func_cond Pointer to function for exit condition from loop.
 *
 * \return Channel list. It will need to be freed.
 *
 * \warning Returned value will need to be freed.
 * 
 */
NMXP_CHAN_LIST *nmxp_getAvailableChannelList(char * hostname, int portnum, NMXP_DATATYPE datatype, int (*func_cond)(void));


/*! \brief Get the list of the start and end time for the available data for each channel.
 *
 * \param hostname host name.
 * \param portnum port number.
 * \param datatype Type of data contained in the channel.
 * \param datas_username DataServer user name.
 * \param datas_password DataServer password.
 * \param flag_request_channelinfo Request information about Network.
 * \param[out] pchannelList pointer to a pointer of channel list.
 * \param func_cond Pointer to function for exit condition from loop.
 *
 * \return Channel list. It will need to be freed.
 *
 * \warning Returned value will need to be freed.
 * 
 */
NMXP_META_CHAN_LIST *nmxp_getMetaChannelList(char * hostname, int portnum, NMXP_DATATYPE datatype, int flag_request_channelinfo, char *datas_username, char *datas_password, NMXP_CHAN_LIST **pchannelList, int (*func_cond)(void));


/*! \brief Base function for qsort() in order to sort an array of pointers to pointers to NMXP_DATA_PROCESS
 *
 * \param a pointer to a pointer to NMXP_DATA_PROCESS
 * \param b pointer to a pointer to NMXP_DATA_PROCESS
 */
int nmxp_raw_stream_seq_no_compare(const void *a, const void *b);


/*! \brief Allocate and initialize fields inside a NMXP_RAW_STREAM_DATA structure
 *
 * \param raw_stream_buffer pointer to NMXP_RAW_STREAM_DATA struct to initialize
 * \param max_tolerable_latency Max tolerable latency
 * \param timeoutrecv value of time-out within receving packets
 *
 */
void nmxp_raw_stream_init(NMXP_RAW_STREAM_DATA *raw_stream_buffer, int32_t max_tolerable_latency, int timeoutrecv);


/*! \brief Free fields inside a NMXP_RAW_STREAM_DATA structure
 *
 * \param raw_stream_buffer pointer to NMXP_RAW_STREAM_DATA struct to initialize
 *
 */
void nmxp_raw_stream_free(NMXP_RAW_STREAM_DATA *raw_stream_buffer);


/*! \brief Execute a list of functions on an chronological ordered array of NMXP_DATA_PROCESS structures
 *
 * \param p pointer to NMXP_RAW_STREAM_DATA
 * \param a_pd pointer to NMXP_DATA_PROCESS struct to insert into the array
 * \param p_func_pd array of functions to execute on a single item NMXP_DATA_PROCESS
 * \param n_func_pd number of functions into the array p_func_pd 
 *
 */
int nmxp_raw_stream_manage(NMXP_RAW_STREAM_DATA *p, NMXP_DATA_PROCESS *a_pd, int (*p_func_pd[NMXP_MAX_FUNC_PD]) (NMXP_DATA_PROCESS *), int n_func_pd);

/*! \brief Execute a list of functions on remaining NMXP_DATA_PROCESS structures
 *
 * \param p pointer to NMXP_RAW_STREAM_DATA
 * \param p_func_pd array of functions to execute on a single item NMXP_DATA_PROCESS
 * \param n_func_pd number of functions into the array p_func_pd 
 *
 */
int nmxp_raw_stream_manage_flush(NMXP_RAW_STREAM_DATA *p, int (*p_func_pd[NMXP_MAX_FUNC_PD]) (NMXP_DATA_PROCESS *), int n_func_pd);

#endif

