//Import express
const express = require('express');
const app = express();//declare an express object (app)
const session = require('express-session'); //for express sessions
const path = require('path');

//Give the app json and url properties
app.use(express.json());
app.use (express.urlencoded({extended:true}));

app.use(session({
    secret: 'mySecret',
    resave: false,
    saveUninitialized: true,
    cookie: { secure: false } // Cambia a true si usas HTTPS
}));

//const admin = require("firebase-admin");
//const credentials = require("./serviceAccountKey.json");

//Assing pug functions to app
app.set('view engine','pug');
app.set('views', path.join(__dirname, 'views'));//"views" is the directory where the pug files are
app.use(express.static(path.join(__dirname, '../public')));
//path.join(__dirname, 'views')

//USER ROUTE IMPORT
const routerUser =require('./routes/user.routes.js');
app.use('/user', routerUser); // /user is the url on which we can access to routerUser

//USER LOGIN ROUTER - this router contains the controller for the login
const routerLogIn =require('./controllers/index.controllers.js');
app.use('/log-in', routerLogIn); // /log-in is the url on which we can access to routerLogIn


//FOR THE ROOT URL
app.use('/',(req,res)=>{//metodo y ruta
    res.render('rootView');
});

/*Solicitud post para Atuthentication
app.post('api/auth', async(req,res)=>{
    const user ={
        email: req.body.email,
        password: req.body.password
    }
    const userResponse= await admin.auth().createUser({
        email: user.email,
        password : user.password,
        emailVerified: false,
        disabled:false
    });
    res.json(userResponse);
});*/
/*
//ARCHIVOS ESTATICOS
app.use(express.static(path.join(__dirname,'../public')));

app.use((req,res)=>{
    res.sendFile((path.join(__dirname,'../public/index.html')));
})*/

//INITIALIZE SERVER ON PORT 3000
app.listen(3000, ()=>{
    console.log('Servidor en espera');
})

