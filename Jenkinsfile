pipeline {
  
  agent any
  
  stages {   
    stage("build") {
      steps {
        sh 'echo "Building Montage develop branch"'
        sh 'pwd'
        sh 'ls'
        sh 'make'
      }
    }
    
    stage("test") {
      steps {
        echo 'Running test (none yet)'
      }
    }
  }
    
  post {
    always {
      sh 'echo "Posting fake results"'
      sh 'junit test/*.xml'
    }
  }
}  
