def buildBadge = addEmbeddableBadgeConfiguration(id: "mBuild", subject: "Montage Build")
def testBadge = addEmbeddableBadgeConfiguration(id: "mTest", subject: "Montage Tests")

pipeline {
  
  agent any
  
  stages {   
    stage("build") {
      steps {
        echo 'Building Montage develop branch'
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
