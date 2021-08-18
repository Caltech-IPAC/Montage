def buildBadge = addEmbeddableBadgeConfiguration(id: "mBuild", subject: "Montage Build")
def testBadge = addEmbeddableBadgeConfiguration(id: "mTest", subject: "Montage Tests")

pipeline {
  
  agent any
  
  stages {   
    stage("build") {
      steps {
        sh 'tests/build.sh'
      }
    }
    
    stage("test") {
      steps {
        sh 'tests/tests.sh'
      }
    }
  }
    
  post {
    always {
      junit 'tests/*.xml'
    }
  }
}  
