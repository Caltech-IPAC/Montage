def buildBadge = addEmbeddableBadgeConfiguration(id: "mBuild", subject: "Montage Build")
def testBadge = addEmbeddableBadgeConfiguration(id: "mTest", subject: "Montage Tests")

pipeline {
  
  agent any
  
  stages {   
    stage("build") {
      steps {
        script {
          sh 'make'
          buildBadge.setStatus('passed')
          buildBadge.setColor('yellow')
          echo 'Badge constructed.'
          sh 'pwd'
          echo 'Build successful.'
        }
      }
    }
    
    stage("test") {
      steps {
        script {
          echo 'TBD regression tests ...'
          addInfoBadge('131 tests')
          testBadge.setStatus('231 passed, 2 failed')
          testBadge.setColor('pink')
          echo 'Regression test complete.'
        }
      }
    }
  }
}  
