pipeline {
  agent {
     node { 
        label 'CentOS8_x86_64'
        } 
  }
  triggers {
  pollSCM('H/30 7-20 * * *')
  }
  stages {
/*
    stage('Checkout scm') {
      steps {
        git 'eolJenkins:ncar/aircraft_rpms'
      }
    }
*/
    stage('Build') {
      steps {
        dir('AltaSetup') { 
          sh('qmake')
          sh('make')
        }
      }
    }
  }
  post {
    failure {
      emailext to: "cjw@ucar.edu janine@ucar.edu cdewerd@ucar.edu",
      subject: "Jenkinsfile aircraft_rpms build failed",
      body: "See console output attached",
      attachLog: true
    }
  }
  options {
    buildDiscarder(logRotator(numToKeepStr: '6'))
  }
}
