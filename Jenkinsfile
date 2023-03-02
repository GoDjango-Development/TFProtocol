pipeline{
    agent any
    stages{
        environment {
            GPG_PASSPHRASE = credentials('GPG_PASSPHRASE')
        }
        stage("Build"){
            steps{
                sh "make clean"
                sh "mkdir -p debug/obj && make debug"
                sh "mkdir -p release/obj && make release"
                sh "git submodule update --init --recursive" // Initialize and download clients submodules for testing purposes
                sh "git secret reveal -p '$GPG_PASSPHRASE' "
                sh "mvn clean -f clients/java" // After this is done we are already ready to run the tests with java client
            }
        }
        
        stage("Test"){
            steps{
                // Testing clients against the server
                sh "debug/tfd conf/example.conf &" // Hoping this work xD i need both this and the next line running at the same time 
                sh "mvn test -f clients/java/" // running the test with the java clients requires you to have a running server so
                // Down here all test should have run successfully so we can pass now to the release stage
            }
        }

        stage("Release"){
            steps{
                sh "docker build -t etherbeing/tfprotocol ." // Builds the docker container as described in the docker file.
                sh "github_deploy.sh release/tfd <<< '${KEY_ID}\
                ${KEY_VALUE}' " // Deploy the tfd file to github releases
            }
        }

        stage("Deploy"){
            steps{
                sh "docker compose up -d"
            }
        }
    }
}