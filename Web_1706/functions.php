<?php
    require 'config.php';

    $userIsAuthN = false;

function isUserAuthenticated($redirect){
    global $userIsAuthN, $maxInactivityPeriod;
    session_start();
    $t=time();
    $diff=0;
    $new=false;
    if (isset($_SESSION['time'])){
        $t0=$_SESSION['time'];
        $diff=($t-$t0);  // inactivity period
    } else {
        $new=true;
    }
    if ($new || ($diff > $maxInactivityPeriod)) { // new or with inactivity period too long
        $_SESSION=array();  //assign an empty array so that $_SESSION gets initialized to nothing

        // If it's desired to kill the session, also delete the session cookie.
        // Note: This will destroy the session, and not just the session data!
        destroySession();

        if($redirect){  //se sono già su index.php non devo redirigere, redirigo solo se sono sulle pagine del profilo
            // redirect client to home page (not the personal page)
            header('HTTP/1.1 307 temporary redirect');
            header('Location: index.php?msg=SessionTimeOut');
            exit; // IMPORTANT to avoid further output from the script
        }
        
    } else {    //user is still authN, so update time of cookie
        $_SESSION['time']=time(); /* update time */
        $userIsAuthN = true;
    }
}

function destroySession(){
    $_SESSION=array();
    if (ini_get("session.use_cookies")) {
        $params = session_get_cookie_params();
        setcookie(session_name(), '', time() - 3600*24, $params["path"], $params["domain"],
            $params["secure"], $params["httponly"]
            ); //sets current non-expired cookie to a date in the past, so that it will be thrown away by the browser
        }
        session_unset();
        session_destroy();  // destroy session
}

function dbConnect(){
    global $host, $user, $pass, $db;

    $conn = new mysqli($host, $user, $pass, $db);
    if($conn->connect_error) {
        redirectWithError("Connection with database failed");
        //die('<div style="color:#fff;background-color:#f44336"><h1>Connection with database failed</h1><h3>Please contact the system administrator</h3></div>');
    }
    if(!$conn) {
        redirectWithError("impossible to connect to database");
        //die('impossible to connect to database');
    }
    // the db credentials are not more needed so unset them for security reasons
    unset($host);
    unset($user);
    unset($pass);
    unset($db);
    $conn->autocommit(false);
    return $conn;
}

function redirect($msg=""){
    header('HTTP/1.1 307 temporary redirect');
    header("Location: index.php?msg=".urlencode($msg));
    exit;
}

function redirectWithError($error){
  header('Location: '.getRedirectionPageError()."?error=".urlencode($error));
  die();
}

function getRedirectionPageError(){
    global $redirectionPages;
    $currentScriptName = basename($_SERVER['SCRIPT_FILENAME']);
    return $redirections[$currentScriptName]['error'];
}

function signup($conn, $email, $password) {
    $result = $conn->query("SELECT uid FROM users WHERE uid = '$email'");
    if(!$result) {  //se il risultato == 0, errore nella query
        redirectWithError('Impossible to create the query');
    }
    if($result->num_rows != 0) {
        // both if password wrong or if non-existing account
        //redirectWithError('This email is already registered.');
        $error = 'This email is already registered.';
        header('Location: index.php?error='.urlencode($error));
        die();
    }
    $result->close(); //free memory of buffer so that the db can be used by other queries
    unset($result);

    $result = $conn->query("INSERT INTO users(uid, upsw) VALUES('$email', '$password')");
    if(!$result) {
        //redirectWithError('Impossible to create the account.');
        $error = 'Impossible to create the account.';
        header('Location: index.php?error='.urlencode($error));
        die();
    }
    // the id of the last inserted value
    $id = $conn->insert_id;
    if(!$conn->commit()) {
        //redirectWithError('Impossible to commit. Please try again');
        $error = 'Impossible to commit. Please try again';
        header('Location: index.php?error='.urlencode($error));
        die();
    }
    // save into the session array
    $_SESSION['time'] = time();
    $_SESSION['email'] = $email;
    $_SESSION['password'] = $password;
    $_SESSION['user_id'] = $id;
    
    header('Location: index.php');
    die();
}

//for large/medium screens
function drawSidebar(){
    global $userIsAuthN;

    if($userIsAuthN){
        //print email of user
        echo '<div class="w3-text-indigo w3-white w3-padding w3-animate-left"><h6>HELLO <br>'.$_SESSION['email'].'<h6></div>';
        //bids
        echo '<a href="index.php" class="w3-bar-item w3-button w3-padding-large w3-indigo w3-hover-white w3-hover-text-indigo">';
        echo '<i class="fa fa-gavel w3-xxlarge"></i>';
        echo '<p>BIDS</p>';
        echo '</a>';
        //my offers
        echo '<a class="w3-bar-item w3-button w3-padding-large w3-hover-white w3-hover-text-indigo w3-text-indigo" href="profile.php">';
        echo '<i class="fa fa-money w3-xxlarge"></i>';
        echo '<p>MY OFFERS</p>';
        echo '</a>';
        //logout
        echo '<a class="w3-bar-item w3-button w3-padding-large w3-hover-white w3-hover-text-indigo w3-text-indigo">';
        echo '<i class="fa fa-external-link-square w3-xxlarge"></i>';
        echo '<p>LOGOUT</p>';
        echo '</a>';
    }else{  //user is not authN
        //bids
        echo '<a href="index.php" class="w3-bar-item w3-button w3-padding-large w3-indigo w3-hover-white w3-hover-text-indigo">';
        echo '<i class="fa fa-gavel w3-xxlarge"></i>';
        echo '<p>BIDS</p>';
        echo '</a>';
        //login
        echo '<button onclick="showLoginCard()" class="w3-bar-item w3-button w3-padding-large w3-hover-white w3-hover-text-indigo w3-text-indigo">';
        echo '<i class="fa fa-user w3-xxlarge"></i>';
        echo '<p>LOGIN</p>';
        echo '</button>';
    }
}

function drawNavbar(){ //for small screens
    global $userIsAuthN;
    if($userIsAuthN){
        echo '<a class="w3-bar-item w3-indigo" style="border-style: solid; border-top:0px; border-left:0px; border-bottom:0px; width:25% !important">HELLO '.$_SESSION['email'].'</a>';
        echo '<a href="index.php" class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important">BIDS</a>';
        echo '<a class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important" href="profile.php">MY OFFERS</a>';
        echo '<a class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important">LOGOUT</a>';
    }else{
        echo '<a href="index.php" class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important">BIDS</a>';
        echo '<a onclick="showLoginCard()" class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important">LOGIN</a>';
    }
}

//forse questa riesce a sostituire il redirectWithError
function drawError(){
    if(isset($_REQUEST['error'])) {
        $error = $_REQUEST['error'];

	    echo '<div class="w3-content w3-card w3-justify w3-text-grey w3-white" id="error">';
  	    echo '<h1 class="w3-center w3-text-red">ERROR</h1>';
    	echo '<p class="w3-center">';
	    echo htmlentities($error);
	    echo '</p>';
    	echo '<p class="w3-indigo w3-hover-white w3-hover-text-indigo w3-center" onclick="hideErrorDIV()">';
    	echo '<i class="fa fa-home w3-large"></i>';
    	echo 'BACK TO HOME';
  	    echo '</p>';
  	    echo '</div>';
	    echo '<div class="w3-row-padding hidden" style="margin:0 -16px" id="page">';
    } else {
        $error = false;
        echo '<div class="w3-row-padding" style="margin:0 -16px" id="page">';
    }
}


?>