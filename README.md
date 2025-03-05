# CMPT_433_Project
Repository for CMPT 433 Term Project

### Description:
A simple single player game where the player controls a tank and tries to destroy enemy tanks. 

### Required Libraries:

```
sudo apt-get install libsfml-dev
```
### Running the program

1) Build The project:
```
$ cmake -B build && cmake --build build
```
2) Run the server (on host):
```
 $ ./build/Server/TankBattleServer
```
3) Run the client (on target):
```
 $ cd /mnt/remote/myApps/Project
 $ ./tank_client
```
