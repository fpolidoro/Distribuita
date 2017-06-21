<?php
    require 'config.php';

    $userIsAuthN = false;

function isUserAuthenticated($redirect){
    global $userIsAuthN, $maxInactivityPeriod;
    session_start();
    $t=time();
    $diff=0;
    $new=false;
    if (isset($_SESSION['time231594'])){
        $t0=$_SESSION['time231594'];
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
            header('Location: index.php?error=SessionTimeOut');
            exit; // IMPORTANT to avoid further output from the script
        }
        
    } else {    //user is still authN, so update time of cookie
        $_SESSION['time231594']=time(); /* update time */
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
        $error = "Connection with database failed";
        header('Location: index.php?fatalError='.urlencode($error));
        die();
    }
    if(!$conn) {
        $error = "Impossible to connect to database";
        header('Location: index.php?fatalError='.urlencode($error));
        die();
    }
    // the db credentials are not more needed so unset them for security reasons
    unset($host);
    unset($user);
    unset($pass);
    unset($db);
    $conn->autocommit(false);
    return $conn;
}

function signup($conn, $email, $password) {
    $result = $conn->query("INSERT INTO users(uid, upsw) VALUES('$email', '$password')");
    if(!$result) {  //if $result == false, then see what the error is, and if it says duplicate entry, it's duplicate email
        if(substr($conn->error, 0, strlen('Duplicate entry')) == 'Duplicate entry'){
            $error = 'The email '. $email . ' is already registered.';
        }else{
            $error = 'An error occurred when creating the account.';
        }
        header('Location: index.php?error='.urlencode($error));
        die();
    }

    if(!$conn->commit()) {
        $error = 'Impossible to commit. Please try again';
        header('Location: index.php?error='.urlencode($error));
        die();
    }
    // save into the session array
    $_SESSION['time231594'] = time();
    $_SESSION['email231594'] = $email;
    $_SESSION['password231594'] = $password;
    
    header('Location: index.php');
    die();
}

function login($conn, $email, $password) {
    //Here select doesn't need to be "for update" because we are just checking whether the user
    //exists and therefore can authN.
    $result = $conn->query("SELECT uid, upsw FROM users WHERE uid = '$email' AND upsw='$password'");
    if(!$result) {  //se il risultato false, errore nella query
        $error = 'Impossible to create the query';
        header('Location: index.php?error='.urlencode($error));
        die();
    }
    if($result->num_rows == 0) {
        // both if password wrong or if non-existing account
        $error = 'Wrong username and/or password.';
        header('Location: index.php?error='.urlencode($error));
        die();
    }

//SELECT returns also a result object
if(!$result->fetch_object()){
    $error = 'Query failed while fetching the result.';
    header('Location: index.php?error='.urlencode($error));
    die();
}

    $result->close(); //free memory of buffer so that the db can be used by other queries
    unset($result);

    // save into the session array
    $_SESSION['time231594'] = time();
    $_SESSION['email231594'] = $email;
    $_SESSION['password231594'] = $password;
    
    //redirect on user's profile
    header('Location: profile.php');
    die();
}

//for large/medium screens
function drawSidebar(){
    global $userIsAuthN;
    $script = basename($_SERVER['SCRIPT_FILENAME']);

    if($userIsAuthN){
        //print email of user
        echo '<div class="w3-text-indigo w3-white w3-padding w3-animate-left"><h6>HELLO <br>'.$_SESSION['email231594'].'</h6></div>';
        //bids
        if($script == 'profile.php'){
            echo '<a href="index.php" class="w3-bar-item w3-button w3-padding-large w3-hover-white w3-hover-text-indigo w3-text-indigo">';
        }else{
            echo '<a href="index.php" class="w3-bar-item w3-button w3-padding-large w3-indigo w3-hover-white w3-hover-text-indigo">';
        }
        echo '<i class="fa fa-gavel w3-xxlarge"></i>';
        echo '<p>BIDS</p>';
        echo '</a>';
        //my offers
        if($script == 'profile.php'){
            echo '<a class="w3-bar-item w3-button w3-padding-large w3-indigo w3-hover-white w3-hover-text-indigo" href="profile.php">';
        }else{
            echo '<a class="w3-bar-item w3-button w3-padding-large w3-hover-white w3-hover-text-indigo w3-text-indigo" href="profile.php">';
        }
        echo '<i class="fa fa-money w3-xxlarge"></i>';
        echo '<p>MY OFFERS</p>';
        echo '</a>';
        //logout
        echo '<a class="w3-bar-item w3-button w3-padding-large w3-hover-white w3-hover-text-indigo w3-text-indigo" href="logout.php">';
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
        echo '<a class="w3-bar-item w3-indigo" style="border-style: solid; border-top:0px; border-left:0px; border-bottom:0px; width:25% !important">HELLO '.$_SESSION['email231594'].'</a>';
        echo '<a href="index.php" class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important">BIDS</a>';
        echo '<a class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important" href="profile.php">MY OFFERS</a>';
        echo '<a class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important" href="logout.php">LOGOUT</a>';
    }else{
        echo '<a href="index.php" class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important">BIDS</a>';
        echo '<a onclick="showLoginCard()" class="w3-bar-item w3-button w3-hover-indigo" style="width:25% !important">LOGIN</a>';
    }
}

function drawBids(){
    global $conn;
    //this select is only for showing the content of the db
    $result = $conn->query("SELECT * FROM bids");
    if(!$result) {  //se il risultato false, errore nella query
        $error = 'Impossible to create the query';
        header('Location: index.php?error='.urlencode($error));
        die();
    }

    if ($result->num_rows > 0) {
        // output data of each row
        while($row = $result->fetch_assoc()) {
            if($row["uid"] == NULL){
                $symbol = "fa-asterisk";
                $str = "starting from:";
                $td = '<td class="w3-dark-grey w3-xlarge w3-padding w3-center hidden" style="width: 25%; height: 100%;"></td><td class="w3-left w3-padding hidden w3-block"><span class="w3-text-indigo w3-center" id="offered_by"></span></td>';
            }else{
                $symbol = "fa-diamond";
                $str = "current bid:";
                $td = '<td class="w3-dark-grey w3-xlarge w3-padding w3-center" style="width: 25%; height: 100%;"></td><td class="w3-left w3-padding w3-block">offered by:<span class="w3-text-indigo w3-center" id="offered_by">'. $row["uid"] .'</span></td>';
            }

            echo '<li>';
            echo '<h1>Bid #'.$row["bid_id"].'</h1>';
            echo '<table class="w3-table w3-white w3-opacity w3-hover-opacity-off w3-center">';
            echo '<tr>';
            echo '<th class="w3-dark-grey w3-xlarge w3-padding w3-center" style="width: 25%; height: 100%;"></th>';
            echo '<th class="w3-dark-grey w3-xlarge" id="cur_text">'. $str .'</th>';
            echo '</tr>';
            echo '<tr>';
            echo '<td class="w3-dark-grey w3-xlarge w3-padding w3-center" style="width: 25%; height: 100%;"><i class="fa '. $symbol .' w3-jumbo"></i></td>';
            echo '<td class="w3-white w3-xxxlarge w3-padding" id="cur_value">'. $row["value"] .' <i class="fa fa-eur w3-xlarge"></i></td>';
            echo '</tr>';
            echo '<tr>';
            echo $td;
            echo '</tr>';
            echo '</table>';
            echo '</li>';
        }
    }else{  //query returned no rows
        $error = 'No bids currently open';
        header('Location: index.php?error='.urlencode($error));
        die();
    }
}

function drawUsersTHR(){
    global $conn;
    $email = $_SESSION['email231594'];
    //join between offers and bids to retrieve the maxbidder and maxbid for each bid
    $result = $conn->query("SELECT offers.bid_id, offers.value AS thr, bids.uid AS maxbidder, bids.value AS maxbid FROM offers INNER JOIN bids ON offers.bid_id = bids.bid_id WHERE offers.uid='$email'");
    if(!$result) {  //se il risultato false, errore nella query
        $error = 'Impossible to create the query';
        header('Location: profile.php?error='.urlencode($error));
        die();
    }

    if ($result->num_rows > 0) {
        // output data of each row
        while($row = $result->fetch_assoc()) {
            echo '<li>';
            echo '<h1>Bid #'. $row['bid_id'] .'</h1>';
            
            echo '<ul class="w3-ul w3-indigo w3-opacity w3-hover-opacity-off"><li><h1>your current offer:</h1></li>';
            echo '<li class="w3-white"><h1 class="w3-large">'. $row["thr"] .'</h1></li>';
            if($row['maxbidder'] == $email){
                echo '<li class="w3-white w3-text-green">you are the highest bidder</li>';
            }else{
              echo '<li class="w3-white w3-text-red">bid exceeded</li>';
            }
            echo '</ul><br>';

            echo '<form action="placebid.php" method="POST"><table class="w3-table w3-white w3-center"><tr><th class="w3-dark-grey w3-xlarge w3-padding w3-center" style="width: 25%; height: 100%;"></th>';
            echo '<th class="w3-dark-grey w3-xlarge" style="border-style: solid; border-right:0px; border-top: 0px; border-bottom:0px; border-color:white" id="cur_text">your new offer:</th></tr><tr>';
            echo '<td class="w3-dark-grey w3-center" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"><i class="fa fa-star w3-jumbo w3-center"></i></td>';
            echo '<td class="w3-white w3-xxlarge" id="cur_value">';
            //input field
            echo '<input type="text" name="thr" id="thr" pattern="[0-9]{1,9}(\.[0-9]{1,2})?" maxlength="13" style="width:75%" placeholder="'.$row['maxbid']. '" required>';
            echo '<i class="fa fa-eur w3-xlarge"></i></td></tr>';
            echo '<tr><td class="w3-dark-grey" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"></td><td class="w3-white">';
            echo '<p class="w3-text-dark-grey" style="margin-top: -4px">enter EUR '. $row["maxbid"] .' or more</p>';
            echo '<p class="hidden" id="maxbid">'.$row["maxbid"].'</p>';
            echo '</td></tr><tr><td class="w3-dark-grey" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"></td>';
            echo '<th class="w3-center w3-large" style="padding: 0px"><button class="w3-button w3-block w3-indigo w3-hover-indigo-light" type="submit" name="placebid">PLACE BID</button></th>';
            echo '</tr></table></form>';   
        	echo '</li>';
        }

        $result->close();
    }else{
    
        //user hasn't placed any thr yet'
        $result = $conn->query("SELECT * FROM bids");
        if(!$result) {  //se il risultato false, errore nella query
            $error = 'Impossible to create the query';
            header('Location: profile.php?error='.urlencode($error));
            die();
        }
        if ($result->num_rows > 0) {
            // output data of each row
            while($row = $result->fetch_assoc()) {
                echo '<li>';
                echo '<h1>Bid #'. $row['bid_id'] .'</h1>';
                
                echo '<ul class="w3-ul w3-indigo w3-opacity w3-hover-opacity-off"><li><h1>your current offer:</h1></li>';
                echo '<li class="w3-white"><h1 class="w3-large">you have not made any offer yet</h1></li>';
                echo '</ul><br>';

                echo '<form action="placebid.php" method="POST"><table class="w3-table w3-white w3-center"><tr><th class="w3-dark-grey w3-xlarge w3-padding w3-center" style="width: 25%; height: 100%;"></th>';
                echo '<th class="w3-dark-grey w3-xlarge" style="border-style: solid; border-right:0px; border-top: 0px; border-bottom:0px; border-color:white" id="cur_text">your new offer:</th></tr><tr>';
                echo '<td class="w3-dark-grey w3-center" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"><i class="fa fa-star w3-jumbo w3-center"></i></td>';
                echo '<td class="w3-white w3-xxlarge" id="cur_value">';
                //input field
                echo '<input type="text" name="thr" id="thr" pattern="[0-9]{1,9}(\.[0-9]{1,2})?" maxlength="13" style="width:75%" placeholder="'.$row['maxbid']. '" required>';
                echo '<i class="fa fa-eur w3-xlarge"></i></td></tr>';
                echo '<tr><td class="w3-dark-grey" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"></td><td class="w3-white">';
                echo '<p class="w3-text-dark-grey" style="margin-top: -4px">enter EUR '. $row["value"] .' or more</p>';
                echo '<p class="hidden" id="maxbid">'.$row["value"].'</p>';
                echo '</td></tr><tr><td class="w3-dark-grey" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"></td>';
                echo '<th class="w3-center w3-large" style="padding: 0px"><button class="w3-button w3-block w3-indigo w3-hover-indigo-light" type="submit" name="placebid">PLACE BID</button></th>';
                echo '</tr></table></form>';   
                echo '</li>';
            }
        }

        $result->close();
    }
}

function computeBid($conn, $thr){
    $result = $conn->query("SELECT * FROM bids FOR UPDATE");
    if(!$result || $result->num_rows == 0) {  //se il risultato false, errore nella query
        $error = 'Impossible to create the query';
        header('Location: profile.php?error='.urlencode($error));
        die();
    }

    //for sake of simplicity, bids contains only an entry, so it is ok to fetch the first row (that'll be the only one)
    $res = $result->fetch_assoc();
    if($thr < $res["value"]){
        $error = 'your offer cannot be lower than the current bid. Your thr='.$thr.'; current bid='.$res["value"];
        $result->close();
        header('Location: profile.php?error='.urlencode($error));
        die();
    }
    $bid_id = $res["bid_id"];
    $curmaxbidder = $res["uid"];
    $email = $_SESSION['email231594'];
    $result->close();
    
    $result = $conn->query("SELECT value FROM offers WHERE uid='$email' FOR UPDATE");
    if(!$result) {  //se il risultato false, errore nella query
        $error = 'Impossible to create the query';
        header('Location: profile.php?error='.urlencode($error));
        die();
    }
    if($result->num_rows == 0){
        $result->close();
        $result = $conn->query("INSERT INTO offers(bid_id, uid, value) VALUES('$bid_id', '$email', '$thr')");
        if(!$result){
            $error = 'Cannot insert your offer.';
            //header('Location: profile.php?error='.urlencode($conn->error));
            header('Location: profile.php?error='.urlencode($error));
            die();
        }
    }else{
        $mycurbid = $result->fetch_assoc();
        $mycurbid = $mycurbid["value"];

        /*if($thr < $mycurbid){
            $error = 'You cannot lower your offer.';
            header('Location: profile.php?error='.urlencode($error));
            die();
        }*/

        $result->close();
        $result = $conn->query("UPDATE offers SET value ='$thr' WHERE bid_id = '$bid_id' AND uid = '$email' ");
        if(!$result){
            $error = 'Cannot insert your offer.';
            header('Location: profile.php?error='.urlencode($conn->error));
            //header('Location: profile.php?error='.urlencode($error)); //DEBUG
            die();
        }
    }

    //compute new bid
    $result = $conn->query("SELECT uid, value FROM offers WHERE bid_id='$bid_id' AND value = (SELECT MAX(value) FROM offers WHERE bid_id = '$bid_id') FOR UPDATE");
    if(!$result || $result->num_rows == 0) {
        $error = 'An issue occurred while computing new bid.';
        header('Location: profile.php?error='.urlencode($conn->error));
        //header('Location: profile.php?error='.urlencode($error)); //DEBUG
        die();
    }
    $res2 = $result->fetch_assoc();
    $maxbidder = $res2["uid"];
    $maxbidders_bid = $res2["value"];


    $result->close();
    //select the second max value among thrs.
    $result = $conn->query("SELECT MAX(value) AS max FROM offers WHERE bid_id='$bid_id' AND uid != '$maxbidder' FOR UPDATE");
    if(!$result || $result->num_rows == 0) {
        $error = 'An issue occurred while computing new bid.';
        header('Location: profile.php?error='.urlencode($conn->error));
        //header('Location: profile.php?error='.urlencode($error)); //DEBUG
        die();
    }

    $res3 = $result->fetch_assoc();
    $maxbid = $res3["max"];

    if($maxbid == NULL){ //there are no other users having thr set, so bid remains unaltered
        $result->close();
        $result = $conn->query("UPDATE bids SET uid ='$maxbidder' WHERE bid_id = '$bid_id'");
        if(!$result) {
            $error = 'An issue occurred while updating bid.';
            header('Location: profile.php?error='.urlencode($conn->error));
            //header('Location: profile.php?error='.urlencode($error)); //DEBUG
            die();
        }
        if(!$conn->commit()){
            $error = 'An issue occurred while committing new bid.';
            header('Location: profile.php?error='.urlencode($conn->error));
            //header('Location: profile.php?error='.urlencode($error)); //DEBUG
            die();
        }
        //successful commit, redirect user to profile page
        header('Location: profile.php');
        die();
    }

    //there are users having thr set, so compute highest bid

    //se trovo che il max ritornato è uguale al thr del maxbidder corrente, allora vuol dire che ci sono due max
    //e devo tenere come maxbidder il maxbidder corrente
    if($maxbid == $maxbidders_bid){
        $maxbidder = $curmaxbidder;
        //$msg = 'maxbid('.$maxbid.') == maxbidders_bid('.$maxbidders_bid.')';  //DEBUG
    }else{
        $maxbid += 0.01;
        //$msg = 'maxbid('.$maxbid.') != maxbidders_bid('.$maxbidders_bid.')';  //DEBUG
    }
    $result->close();
    $result = $conn->query("UPDATE bids SET uid ='$maxbidder', value='$maxbid' WHERE bid_id = '$bid_id'");
    if(!$result) {
        $error = 'An issue occurred while updating bid. UPDATE bids. Users with THR set';
        header('Location: profile.php?error='.urlencode($conn->error));
        //header('Location: profile.php?error='.urlencode($error));
        die();
    }
    
    if(!$conn->commit()){
        $error = 'An issue occurred while committing new bid.';
        header('Location: profile.php?error='.urlencode($conn->error));
        //header('Location: profile.php?error='.urlencode($error)); //DEBUG
        die();
    }
    //successful commit, redirect user to profile page
    //header('Location: profile.php?msg='.urlencode($msg));    //DEBUG
    header('Location: profile.php');
    die();
}
?>
