<?php
    require 'config.php';

    $userIsAuthN = false;

function isUserAuthenticated(){
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

        // redirect client to home page (not the personal page)
        header('HTTP/1.1 307 temporary redirect');
        header('Location: index.php?msg=SessionTimeOut');
        exit; // IMPORTANT to avoid further output from the script
        
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
        session_destroy();  // destroy session
}

function dbConnect(){
    global $host, $user, $pass, $db;

    $conn = mysql_connect($host, $user, $pass, $db);
    if($conn->connect_error) {
        die('<div style="color:#fff;background-color:#f44336"><h1>Connection with database failed</h1><h3>Please contact the system administrator</h3></div>');
    }
    if(!$conn) {
        die('impossible to connect to database');
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
    header("Location: home.php?msg=".urlencode($msg));
    exit;
}

function redirectWithError($error){
  header('Location: '.getRedirectionPageError()."?error=$error");
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
    if($result->num_rows == 0) {
        // both if password wrong or if non-existing account
        redirectWithError('This email is already registered.');
    }
    $result->close(); //free memory of buffer so that the db can be used by other queries
    unset($result);

    $result = $conn->query("INSERT INTO users(uid, upsw) VALUES('$email', '$password')");
    if(!$result) {
        redirectWithError('Impossible to create the account.');
    }
    // the id of the last inserted value
    $id = $conn->insert_id;
    if(!$conn->commit()) {
        redirectWithError('Impossible to commit. Please try again');
    }
    // save into the session array
    $_SESSION['time'] = time();
    $_SESSION['email'] = $email;
    $_SESSION['password'] = $password;
    $_SESSION['user_id'] = $id;
    // and go to the right place
    goToDestination();
}
?>