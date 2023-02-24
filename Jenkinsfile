pipeline{
    agent any
    stages{
        stage("Build"){
            steps{
                sh "echo hello world"
            }
        }
        
        stage("Test"){
            steps{
                sh "echo Test"
            }
        }

        stage("Release"){
            steps{
                sh "echo Release"
            }
        }

        stage("Deploy"){
            steps{
                sh "Deployment"
            }
        }
    }
}