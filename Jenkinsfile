Jenkinsfile (Declarative Pipeline)

pipeline{
    agent any
    environment {
        // Using returnStdout
        CC = """${sh(
                returnStdout: true,
                script: 'echo "clang"'
            )}""" 
        // Using returnStatus
        EXIT_STATUS = """${sh(
                returnStatus: true,
                script: 'exit 1'
            )}"""
    }
    stages{
        stage("Build"){
            environment {
                DEBUG_FLAGS = '-g'
            }
            steps{
                sh "echo hello world :)"
            }
        }
        
        stage("Test"){
            steps{}
        }

        stage("Release"){
            steps{}
        }

        stage("Deploy"){
            steps{}
        }
    }
}