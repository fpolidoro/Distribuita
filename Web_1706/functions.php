<?php
    require 'config.php';

function isUserAuthenticated(){
    global $authenticated, $maxInactivityPeriod;
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
        // redirect client to login page
        header('HTTP/1.1 307 temporary redirect');
        header('Location: login.php?msg=SessionTimeOut');
        exit; // IMPORTANT to avoid further output from the script
    } else {
        $_SESSION['time']=time(); /* update time */
        echo '<html><body>Updated last access time: '.$_SESSION['time'].'</body></html>';
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


?>