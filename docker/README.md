# Build and test CTF inside a Docker container

1. Pull the latest version of the Docker image that has the CTF environment and
   the TPLs already pre-installed:
    ```
    $ docker pull docker.io/agabgim/ctf_tpls_rdfmgcodes:final
    ```
   The image is hosted on Docker hub. You may look at the `Dockerfile` that was
   use to build it. It is located in the `docker/` directory of the CTF TPLS
   source repository.

2. Start your container with:
    ```
    $ CONTAINER_CTF_INSTALL_DIR=/tools/ctf/installs/`date +%Y-%m-%d`; \
      CONTAINER_CTF_DIR=/scratch/ctf_source; \
      docker run -d \
        --name <give your container a name> \
        --env CTF_INSTALL_DIR=${CONTAINER_CTF_INSTALL_DIR} \
        --env CTF_DIR=${CONTAINER_CTF_DIR} \
        --volume <path to CTF on the host machine>:${CONTAINER_CTF_DIR} \
        docker.io/agabgim/ctf_tpls_rdfmgcodes:final tail -f /dev/null
    ```
   This will mount your local ctf source tree into the container at
   `/scratch/ctf_source` and will run the container in the background here with
   the commmand `tail -f /dev/null` that will never exit unless you kill it.
   Then connect to the container by doing:
    ```
    $ docker exec -it <container name> bash
    ```
   This will launch an interactive Bash shell inside it as the root user. The
   `-it` instructs Docker to allocate a pseudo-TTY connected to your STDIN.
   Note that the base image is built on top of `ubuntu` so you may run
   `apt-get update && apt-get install ...` if you like to install any package.
   You may edit the source from the container since the volume is mounted in
   read-write mode here but it might be easier to do so directly on your local
   system.

3. You should see the command prompt inside the container. Run these commands:
    ```
    root@af8bae53bdd3:/# cd $CTF_BUILD_DIR && ./do_configure.sh
    ```
   The `CTF_BUILD_DIR` environment variable is already defined and a configure
   script has been copied into that directory. Then, do `make -j<N>` and `ctest
   -j<N>` as you would usually do.

4. If you wish to commit your changes into a new image, use:
    ```
    $ docker commit <container name> <choose a name for the new image>
    ```
   You might want to remember to clean up disk space and delete the scratch
   space beforehand by doing:
    ```
    $ docker exec -it rm -f ${CTF_BUILD_DIR}
    ```
   Alternatively, you can save your CTF image into an `.tar.gz` archive using:
    ```
    $ docker export <container name> | gzip > <tarball name>
    ```
   To turn the archive back into a Docker image on another system, you would do:
    ```
    $ gzcat <archive name> | docker import - <image name>
    $ docker run -it <image name>
    ```

5. Clean up after yourself! You can see all the containers with:
    ``` 
    $ docker ps -a
    ```
   Use the `docker stop` and `docker rm` commands to get rid of the containers
   you don't use any more.
