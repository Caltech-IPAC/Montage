pipeline {
  
  agent any
  
  stages {   
    stage("build") {
      steps {
        echo 'Building Montage develop branch'
        pwd
        ls
        make
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
      echo 'Posting results (none yet)'
    }
  }
}  
