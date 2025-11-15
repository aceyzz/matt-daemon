<img title="42_matt-daemon" alt="42_matt-daemon" src="./utils/banner.png" width="100%">

<br>

# Matt-Daemon — 42

Ce projet a pour but de réaliser un daemon en C++ qui écoute sur le port 4242 en enregistrant les messages reçus dans un fichier log.

<br>

## Description

Serveur TCP daemonisé écrit en C++11 qui accepte des connexions entrantes et enregistre l'ensemble des opérations dans un fichier journal (`matt_daemon.log`). Le daemon se met en arrière-plan, gère les signaux système (`SIGINT`, `SIGTERM`, `SIGHUP`, `SIGQUIT`) pour un arrêt propre, et implémente un système de rotation de logs avec limitation du nombre de fichiers de sauvegarde.

### Architecture

Le projet repose sur trois composants principaux :

**Daemon** : Orchestrateur central qui effectue la daemonisation en deux étapes, acquiert un verrou fichier pour éviter les instances multiples, gère les signaux et lance le serveur. Les deux fork garantissent que le processus se détache complètement du terminal parent et devient une session indépendante.

**Server** : Serveur TCP non-bloquant utilisant `select()` pour gérer plusieurs connexions client simultanément. Chaque client reçoit un identifiant unique et dispose de buffers d'entrée/sortie pour gérer la fragmentation réseau. Les lignes de commande sont traitées lors de la réception de `\n`, avec support du caractère `\r` (CRLF).

**Tintin_reporter** : Système de journalisation qui écrit dans `/var/log/matt_daemon/matt_daemon.log` avec rotation automatique à 1 MB. Les anciens logs sont renommés avec un timestamp et les fichiers en excédent (au-delà de 5 sauvegardes) sont supprimés automatiquement. Chaque ligne inclut un timestamp au format `DD/MM/YYYY-HH:MM:SS` et un niveau (LOG, INF, WRN, ERR).  
La rotation des logs n'est pas un élément obligatoire du projet (concerne partie bonus), mais elle a été implémentée pour améliorer la robustesse et la maintenabilité du daemon.

**FileOps** : Couche d'abstraction pour les opérations fichier (création, lecture, écriture, suppression, verrouillage exclusif) avec gestion des erreurs `EINTR` et création récursive de répertoires.

### Caractéristiques techniques

Le serveur accepte jusqu'à 3 clients simultanés et ferme les nouvelles connexions avec un message d'erreur si cette limite est atteinte. Les options socket appliquées incluent SO_REUSEADDR, SO_REUSEPORT et TCP_KEEPALIVE pour favoriser le redémarrage rapide et la stabilité. Le timeout de sélection est de 100ms pour permettre la vérification périodique des signaux arrêt.

La commande `quit` permet au client de gracieux arrêter le daemon. Les connexions fermées ou en erreur sont loggées avec le niveau et la raison de déconnexion. Les sockets sont configurés en non-bloquant et utilisent SO_LINGER pour forcer la fermeture immédiate des connexions.

<br>

## Utilisation

### Compilation

Lancer l'environnement sandbox avec Docker
```bash
cd project
docker-compose up -d
docker-compose exec -it matt-daemon bash
```

Une fois dans le docker, compilation du projet
```bash
make			  # Compile le daemon
# autres commandes make disponibles :
make clean		  # Supprime les fichiers objets
make fclean		  # Supprime les fichiers objets et l'exécutable
make re			  # Recompile à partir de zéro
make clean-logs	  # Supprime les logs générés
make leaks		  # Exécute le daemon avec valgrind pour détecter les fuites mémoire
make monitor	  # Lance un tail -f sur le fichier de log du daemon
```

### Lancement

Le daemon doit être exécuté en tant que root car il nécessite l'accès à `/var/log` et `/var/lock`.

```bash
sudo su
./MattDaemon
```

Une fois lancé, le daemon se met en arrière-plan et enregistre les opérations dans `/var/log/matt_daemon/matt_daemon.log`. Un fichier verrou est créé dans `/var/lock/matt_daemon.lock` et contient le PID du processus en cours.

### Test

Le projet inclut un script de test pour les logs avancé qui valide la rotation des logs en envoyant des données volumineux via netcat :

```bash
cd project/code
chmod +x log-tester.sh
sudo ./log-tester.sh
```

Ce script teste l'acceptation de connexions, l'envoi de grandes lignes pour déclencher la rotation, et la limitation du nombre de fichiers de sauvegarde.

Pour envoyer manuellement des données au daemon :

```bash
echo "Test de message" | nc 127.0.0.1 4242
echo "quit" | nc 127.0.0.1 4242  # Arrête le daemon
```

Ou de manière interactive :

```bash
nc 127.0.0.1 4242
```


<br>

## Choix d'implémentation

**Daemonisation classique** : Deux fork successifs suivis d'un `setsid()` pour créer une nouvelle session détachée du terminal. Redirection des descripteurs standard vers `/dev/null` et changement du répertoire courant vers `/`.

**Verrou fichier** : Utilisation de `flock()` en mode non-bloquant pour éviter plusieurs instances. Le verrou est acquis avant la daemonisation et le fichier est supprimé à l'arrêt.

**I/O non-bloquant** : `select()` gère les lectures et écritures sur tous les descripteurs avec un timeout court. Améliore la réactivité pour l'arrêt sur signal et évite les deadlocks.

**Gestion des signaux** : Un fonction statique `_onSignal()` positionne un flag global qui est vérifiée régulièrement. Évite les appels système non-sûrs dans le signal handler.

**Rotation des logs** : Implémentée avant chaque écriture en vérifiant la taille projeta. Les fichiers archivés sont listés, triés et les plus anciens supprimés en cas de dépassement du nombre maximal.

**Classes Coplien** : Les classes implémentent les constructeurs de copie et l'assignation (souvent désactivés) pour éviter les problèmes de gestion de ressources.

<br>

## Liens utiles

- [Sujet officiel](./utils/en.subject.pdf)

<br>

## Grade

<img src="./utils/85.png" alt="Grade" width="150">

Deux soucis ont ete remontés lors de la soutenance :  
- Il faut explicitement utiliser `LOCK_EX` et `LOCK_UN` avec `flock()` pour le verrouillage du fichier afin d'éviter les blocages. `LOCK_EX` pour le verouillage a été implémenté, mais le `LOCK_UN` ne me semblait pas necessaire car le fichier de lock est supprimé a la fin du programme, ce qui libere automatiquement le verrou. Or, il est explicitement demandé dans la feuille d'evaluation d'utiliser `LOCK_UN` (tres discutable). Correction faite dans la version finale.  
- Lorsque le nombre d'utilisateurs connectés dépasse la limite, un message d'erreur explicite doit etre affiché (côté client et/ou logs serveur). Dans mon implementation initiale, aucun des deux n'a ete fait = faux. Correction faite dans la version finale, avec log serveur + message client.

<br>

## Auteurs

<table>
	<tr>
		<td align="center">
			<a href="https://github.com/cduffaut" target="_blank">
				<img src="https://img.icons8.com/?size=100&id=tZuAOUGm9AuS&format=png&color=000000" width="40" alt="GitHub Icon"/><br>
				<b>Cécile</b>
			</a>
		</td>
		<td align="center">
			<a href="https://github.com/aceyzz" target="_blank">
				<img src="https://img.icons8.com/?size=100&id=tZuAOUGm9AuS&format=png&color=000000" width="40" alt="GitHub Icon"/><br>
				<b>Cédric</b>
			</a>
		</td>
	</tr>
</table>
