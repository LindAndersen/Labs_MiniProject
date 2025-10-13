# SETUP
**MacOS**
<br/>
export DOCKER_DEFAULT_PLATFORM=linux/amd64

**Windows**
<br/>
set DOCKER_DEFAULT_PLATFORM=linux/amd64

docker build -t ubuntu-cyber-machine .


# Using
Linking files with container files

**MacOS**
<br/>
```sh
docker run -ti -v $(pwd):/home -w /home ubuntu-cyber-machine bash
```

**Windows**
<br/>
```sh
docker run -ti -v C:\Users\YourName\project:/home -w /home ubuntu-cyber-machine bash
```


<br/><br/><br/><br/><br/><br/>





docker run -ti -v C:\Users\frede\"OneDrive - Syddansk Universitet"\"5. Semester"\"Cyber Security"\"C++ Cybersecurity group exercise":/home -w /home ubuntu-cyber-machine bash


