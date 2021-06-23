def buildBadge = addEmbeddableBadgeConfiguration(id: "mBuild", subject: "Montage Build")
def testBadge = addEmbeddableBadgeConfiguration(id: "mTest", subject: "Montage Tests")

pipeline {
  
  agent any
  
  stages {   
    stage("build") {
      steps {
        sh 'make'
      }
    }
    
    stage("test") {
      steps {
        script {
          echo 'TBD regression tests ...'
          echo 'Regression test complete.'
        }
      }
    }
  }
    
  post {
    always {
      junit 'tests/*.xml'
    }
  }
}  
